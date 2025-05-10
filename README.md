<p align="center">
  <img src="aurora-logo.svg" width="100">
</p>
<p align="center">
  <img src="aurora-wordmark.svg">
</p>
<h3 align="center">An open-source chess engine with an MCTS-Minimax hybrid search and an NNUE evaluation</h3>

# General

Aurora is an actively developed UCI (Universal Chess Interface) compatible chess engine.
This means that Aurora is a command-line-based engine and you will need a UCI-Compatible GUI in order to intuitively use Aurora. See the "Usage" section below for usage instructions.

See [Playing Strength Testing](https://github.com/kjljixx/Aurora-Chess-Engine/wiki/Testing#playing-strength) for Aurora's playing strength.

## Usage

To use Aurora, simply download a binary from the [latest release](https://github.com/kjljixx/Aurora-Chess-Engine/releases), for example on Windows you would download ```aurora-x-windows.exe```, where x is the version of the latest release. Go to a chess GUI of your choice, and find instructions for adding engines, and add this binary.

If you want to build Aurora from the source code, download the source code, navigate to the directory, and run
```shell
make
```
You should now have a file named ```aurora.exe```, which is the engine. Now go to your GUI and find instructions for adding engines. When the GUI prompts you to choose a file/engine, choose ```aurora.exe```.

See an explanation of all the options [on the wiki](https://github.com/kjljixx/Aurora-Chess-Engine/wiki/Options)

# Technical Details
Aurora uses a UCT (Upper Confidence Bounds applied to Trees) search with an NNUE (Efficiently Updatable Neural Network)

## Search
Here are some details about the search:
* Builds a search tree with nodes
* Nodes to expand are selected with UCT
* Values backpropagated up the tree are done so with minimax (rather than averaging the values, as with traditional UCT)

## Evaluation
A NNUE trained with [bullet](https://github.com/jw1912/bullet) and options (the options are for ```andromeda-3.nnue```):
* Arch:  (768->256)x2->1
* Scale: 400
* Batch Size: 16384
* Batches / Superbatch: 6104
* Positions / Superbatch: 100007936
* Superbatches: 88
* WDL scheduler: constant 0.0
* LR scheduler: start 0.001 gamma 0.1 drop every 44 superbatches
* Device: NVIDIA GeForce MX550
* Threads: 4
* Positions: 154143704
