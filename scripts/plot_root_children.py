#!/usr/bin/env python3
"""Interactive Dash app for root-child search stats CSV dumps."""

from __future__ import annotations

import argparse
from pathlib import Path

import pandas as pd
import plotly.express as px
from dash import Dash, Input, Output, dcc, html

Y_CHOICES = ("variance", "std_dev", "avg", "q", "visits", "iters")
X_CHOICES = ("root_iters", "root_visits", "iters", "visits")

# Sentinels written for missing / uninitialized child or edge fields in the C++ dump.
Y_SENTINELS = {
  "avg": {-2.0},
  "q": {-2.0},
  "variance": {-1.0},
  "std_dev": {-1.0},
  "iters": {0},
  "visits": set(),  # visits=1 is both pruned sentinel and a real 1-visit child
}


def parse_args() -> argparse.Namespace:
  parser = argparse.ArgumentParser(
    description="Dash app: plot root-child metrics vs root iters/visits from Aurora DEV CSV dumps."
  )
  parser.add_argument(
    "csv",
    nargs="?",
    default="search_stats_root_children.csv",
    help="Path to CSV (default: search_stats_root_children.csv)",
  )
  parser.add_argument("--x", choices=X_CHOICES, default="root_iters", help="Initial x axis")
  parser.add_argument("--y", choices=Y_CHOICES, default="variance", help="Initial y axis")
  parser.add_argument("--host", default="127.0.0.1")
  parser.add_argument("--port", type=int, default=8050)
  return parser.parse_args()


def filter_defaults(df: pd.DataFrame, y_col: str) -> pd.DataFrame:
  out = df
  if "pruned" in out.columns:
    out = out[out["pruned"] == 0]
  sentinels = Y_SENTINELS.get(y_col, set())
  if sentinels:
    out = out[~out[y_col].isin(sentinels)]
  if "q" in out.columns and y_col != "q":
    out = out[out["q"] != -2.0]
  if "avg" in out.columns and y_col != "avg":
    out = out[out["avg"] != -2.0]
  return out


def make_figure(df: pd.DataFrame, x_col: str, y_col: str, log_x: bool = False, log_y: bool = False):
  filtered = filter_defaults(df, y_col)
  if log_x:
    filtered = filtered[filtered[x_col] > 0]
  if log_y:
    filtered = filtered[filtered[y_col] > 0]
  if filtered.empty:
    fig = px.line(title=f"No data after filtering ({y_col} vs {x_col})")
    fig.update_layout(xaxis_title=x_col, yaxis_title=y_col)
    return fig

  hover_cols = [c for c in ("move", "q", "avg", "iters", "visits", "variance", "std_dev") if c in filtered.columns]
  fig = px.line(
    filtered,
    x=x_col,
    y=y_col,
    color="move",
    title=f"{y_col} vs {x_col}",
    hover_data=hover_cols,
    log_x=log_x,
    log_y=log_y,
  )
  fig.update_traces(mode="lines")
  fig.update_layout(legend_title_text="move")
  return fig


def main() -> None:
  args = parse_args()
  path = Path(args.csv)
  if not path.is_file():
    raise SystemExit(f"CSV not found: {path}")

  df = pd.read_csv(path)
  missing = [c for c in ("move", *X_CHOICES, *Y_CHOICES) if c not in df.columns]
  if missing:
    raise SystemExit(f"CSV missing columns: {sorted(set(missing))}")

  x_options = [{"label": c, "value": c} for c in X_CHOICES]
  y_options = [{"label": c, "value": c} for c in Y_CHOICES]

  app = Dash(__name__)
  app.title = "Aurora root children"
  app.layout = html.Div(
    style={"fontFamily": "sans-serif", "margin": "1.5rem"},
    children=[
      html.H2("Aurora root-child search stats"),
      html.P(f"Source: {path.resolve()}"),
      html.Div(
        style={"display": "flex", "gap": "1.5rem", "marginBottom": "1rem", "alignItems": "flex-end"},
        children=[
          html.Div(
            [
              html.Label("X axis"),
              dcc.Dropdown(id="x-axis", options=x_options, value=args.x, clearable=False),
              dcc.Checklist(
                id="log-x",
                options=[{"label": " log scale", "value": "log"}],
                value=[],
                style={"marginTop": "0.4rem"},
              ),
            ],
            style={"minWidth": "220px"},
          ),
          html.Div(
            [
              html.Label("Y axis"),
              dcc.Dropdown(id="y-axis", options=y_options, value=args.y, clearable=False),
              dcc.Checklist(
                id="log-y",
                options=[{"label": " log scale", "value": "log"}],
                value=[],
                style={"marginTop": "0.4rem"},
              ),
            ],
            style={"minWidth": "220px"},
          ),
        ],
      ),
      dcc.Graph(id="root-children-graph", style={"height": "75vh"}),
    ],
  )

  @app.callback(
    Output("root-children-graph", "figure"),
    Input("x-axis", "value"),
    Input("y-axis", "value"),
    Input("log-x", "value"),
    Input("log-y", "value"),
  )
  def update_graph(x_col: str, y_col: str, log_x_vals: list, log_y_vals: list):
    return make_figure(df, x_col, y_col, log_x="log" in (log_x_vals or []), log_y="log" in (log_y_vals or []))

  print(f"Dash app: http://{args.host}:{args.port}  (csv={path})")
  app.run(host=args.host, port=args.port, debug=False)


if __name__ == "__main__":
  main()
