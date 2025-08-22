#!/usr/bin/env python3
"""
Log Analysis Script for Aurora Chess Engine

This script analyzes log files in the /logs/ directory and creates a histogram
of evaluation values vs game results for positions where the number of nodes
searched was between 450 and 500.

Game results are inferred from the last evaluation value before a game ends:
- Values close to 1.0 -> Win (result = 1)
- Values close to 0.0 -> Draw (result = 0)  
- Values close to -1.0 -> Loss (result = -1)
"""

import os
import glob
import matplotlib.pyplot as plt
import math
import numpy as np
import warnings
from scipy.optimize import curve_fit, OptimizeWarning


def infer_game_result(last_value):
    """
    Infer game result from the last evaluation value.
    
    Args:
        last_value (float): Last evaluation value in range [-1, 1]
        
    Returns:
        int: Game result (-1 for loss, 0 for draw, 1 for win)
    """
    if last_value > 0.33:
        return 1  # Win
    elif last_value < -0.33:
        return -1  # Loss
    else:
        return 0  # Draw


def scaled_sigmoid(x, a):
    return a / (1 + np.exp(math.log1p((2 * a) / (2 + a) - 2) * x)) - a/2


def create_bucketed_plot(games_data=None):
    """
    Analysis for all ranges of 50 up to 500, with sigmoid fit.
    Single pass through files: update all ranges/buckets as we go.
    """
    logs_dir = "logs"
    print(f"Looking for log files in '{logs_dir}'...")
    log_files = glob.glob(os.path.join(logs_dir, "*"))
    print(f"Found {len(log_files)} log files.")
    num_buckets = 30
    bin_edges = np.linspace(-1, 1, num_buckets + 1)
    bin_centers = (bin_edges[:-1] + bin_edges[1:]) / 2
    ranges = [(start, start+49) for start in range(50, 501, 50)] + [(10000, 11000)]
    # For each range, keep bucket sums/counts
    range_bucket_sum_results = {rng: [0.0]*num_buckets for rng in ranges}
    range_bucket_count_results = {rng: [0]*num_buckets for rng in ranges}
    range_total_positions = {rng: 0 for rng in ranges}
    for idx, filepath in enumerate(log_files):
        print(f"Reading file {idx+1}/{len(log_files)}: {filepath}")
        try:
            with open(filepath, 'r') as f:
                lines = f.readlines()
        except Exception as e:
            print(f"Error reading {filepath}: {e}")
            continue
        i = 0
        last_value = None
        current_game_pairs = []
        game_count = 0
        while i < len(lines):
            line = lines[i].strip()
            if line == "NEWGAME":
                game_count += 1
                if current_game_pairs and last_value is not None:
                    game_result = infer_game_result(last_value)
                    for nodes, value in current_game_pairs:
                        for rng in ranges:
                            node_min, node_max = rng
                            if node_min <= nodes <= node_max:
                                for b in range(num_buckets):
                                    if bin_edges[b] <= value < bin_edges[b+1] or (b == num_buckets-1 and value == bin_edges[-1]):
                                        range_bucket_sum_results[rng][b] += game_result
                                        range_bucket_count_results[rng][b] += 1
                                        range_total_positions[rng] += 1
                                        break
                current_game_pairs = []
                last_value = None
                i += 1
            elif line == "NEWSEARCH":
                i += 1
                if i < len(lines):
                    search_line = lines[i].strip()
                    tokens = search_line.split()
                    for j in range(0, len(tokens) - 1, 2):
                        try:
                            num_nodes = int(tokens[j])
                            value = float(tokens[j + 1])
                            last_value = value
                            current_game_pairs.append((num_nodes, value))
                        except Exception:
                            continue
                i += 1
            else:
                i += 1
        if current_game_pairs and last_value is not None:
            game_result = infer_game_result(last_value)
            for nodes, value in current_game_pairs:
                for rng in ranges:
                    node_min, node_max = rng
                    if node_min <= nodes <= node_max:
                        for b in range(num_buckets):
                            if bin_edges[b] <= value < bin_edges[b+1] or (b == num_buckets-1 and value == bin_edges[-1]):
                                range_bucket_sum_results[rng][b] += game_result
                                range_bucket_count_results[rng][b] += 1
                                range_total_positions[rng] += 1
                                break
        print(f"  Processed {game_count} games in this file.")
    # Plotting and fitting
    all_range_stats = []
    plt.figure(figsize=(14, 10))
    for r_idx, rng in enumerate(ranges):
        node_min, node_max = rng
        print(f"\nProcessing node range {node_min}-{node_max}...")
        bucket_averages = [range_bucket_sum_results[rng][i]/range_bucket_count_results[rng][i] if range_bucket_count_results[rng][i] > 0 else np.nan for i in range(num_buckets)]
        bucket_counts = range_bucket_count_results[rng]
        total_positions = range_total_positions[rng]
        valid_indices = [i for i, count in enumerate(bucket_counts) if count > 0]
        plot_centers = [bin_centers[i] for i in valid_indices]
        plot_averages = [bucket_averages[i] for i in valid_indices]
        plot_counts = [bucket_counts[i] for i in valid_indices]
        plt.errorbar(plot_centers, plot_averages, fmt='o-', capsize=3, capthick=1, linewidth=2, markersize=4, label=f'Nodes {node_min}-{node_max}')
        # Fit sigmoid curve (one parameter, a > 2)
        fit_stats = None
        if len(plot_centers) >= 4:
            try:
                print("  Fitting sigmoid curve (basic, 1 param, a>2)...")
                initial_guess = [3.0]
                popt, pcov = curve_fit(scaled_sigmoid, plot_centers, plot_averages, p0=initial_guess, bounds=([2.0001], [np.inf]), maxfev=2000)
                x_smooth = np.linspace(-1, 1, 200)
                y_smooth = scaled_sigmoid(x_smooth, *popt)
                plt.plot(x_smooth, y_smooth, '-', linewidth=2, alpha=0.8)
                y_pred = scaled_sigmoid(np.array(plot_centers), *popt)
                ss_res = np.sum((np.array(plot_averages) - y_pred) ** 2)
                ss_tot = np.sum((np.array(plot_averages) - np.mean(plot_averages)) ** 2)
                r_squared = 1 - (ss_res / ss_tot) if ss_tot > 0 else 0
                print(f"  Sigmoid fit parameter: a={popt[0]:.3f}")
                print(f"  R²: {r_squared:.3f}")
                fit_stats = {'range': f'{node_min}-{node_max}', 'a': popt[0], 'r2': r_squared, 'positions': total_positions}
            except Exception as e:
                print(f"  Warning: Could not fit sigmoid curve: {e}")
        else:
            print("  Not enough data points for sigmoid fitting.")
        all_range_stats.append(fit_stats)
    plt.xlabel('Evaluation Value')
    plt.ylabel('Average Game Result')
    plt.title('Average Game Result vs Evaluation Value (All Node Ranges)')
    plt.grid(True, alpha=0.3)
    plt.ylim(-1.1, 1.1)
    plt.axhline(y=0, color='gray', linestyle='-', alpha=0.5)
    plt.axhline(y=1, color='green', linestyle='--', alpha=0.3)
    plt.axhline(y=-1, color='red', linestyle='--', alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig('evaluation_analysis_all_ranges.png', dpi=300, bbox_inches='tight')
    print("Plot saved as 'evaluation_analysis_all_ranges.png'.")
    plt.show()
    print("\nSummary of sigmoid fits:")
    for stat in all_range_stats:
        if stat:
            print(f"  Range {stat['range']}: a={stat['a']:.3f}, R²={stat['r2']:.3f}, positions={stat['positions']}")


def main():
    """Main function to run the log analysis."""
    create_bucketed_plot()

if __name__ == "__main__":
    main()
