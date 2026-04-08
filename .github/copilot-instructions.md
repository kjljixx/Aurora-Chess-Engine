# Aurora Chess Engine Guidelines

## Build And Run
- Build from repo root with `make`.
- `make` uses `clang++` and targets `-march=x86-64-v3` with C++17.
- Run the engine with `./aurora` (or `aurora.exe` on Windows).
- Run benchmark with `aurora bench`.
- Before committing changes, ALWAYS run `aurora bench` and append `bench xx` (where xx is the number of nodes searched) output to the commit message.

## Architecture
- Entry point: `aurora.cpp`.
- UCI protocol loop and command handling: `uci.h`.
- Search/tree/time management: `search.h`.
- Evaluation and NNUE inference: `evaluation.h`.
- Board state, rules, and move legality: `chess.h`.
- Bitboard utilities and attack helpers: `bitboards.h`, `lookup.h`, `rays.h`.
- Hashing: `zobrist.h`.

## Project Conventions
- This codebase is header-heavy and uses many `inline` definitions for engine logic and globals; preserve existing placement/style unless there is a clear refactor goal.
- Keep UCI option names and semantics stable (`Hash`, `TTHash`, `Threads`, `SyzygyPath`, and tuning options in `aurora.h`).
- When editing search behavior, verify interactions with tree reuse and transposition-table sizing in `search.h`.

## Pitfalls
- `DATAGEN` in `aurora.h` must be `0` for normal engine builds (`aurora.cpp` warns and exits otherwise).
- `andromeda-3.nnue` is compiled in via `INCBIN` in `evaluation.h`; do not rename/move it without updating code.
- Avoid using Syzygy tablebases

## Documentation
- General usage and technical overview: `README.md`.
- Do NOT use or edit code in `weather-factory`, `trainer`, or `engine/EngineTests`
