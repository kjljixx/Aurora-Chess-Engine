# Aurora
![Aurora's logo, generated by Bing Image Creator](https://github.com/kjljixx/Aurora-Chess-Engine/blob/main/Aurora.jpg)
<p style='text-align: right;'>Logo generated by Bing Image Creator</em></p>

# General

Aurora is an actively developed UCI (Universal Chess Interface) compatible chess engine.
This means that Aurora is a command-line-based engine and you will need a UCI-Compatible GUI in order to intuitively use Aurora. See the "Usage" section below for usage instructions.

See [Playing Strength Testing](https://github.com/kjljixx/Aurora-Chess-Engine/wiki/Testing#playing-strength) for Aurora's playing strength.

## Usage

To use Aurora, simply download a binary from the [latest release](https://github.com/kjljixx/Aurora-Chess-Engine/releases), for example on Windows you would download ```aurora-x-windows-2022.exe```, where x is the version of the latest release. Go to a chess GUI of your choice, and find instructions for adding engines, and add this binary.

If you want to build Aurora from the source code, download the source code, navigate to the directory, and build ```aurora.cpp```. For example, if you use g++, run the following command (with any compiler flags you want in addition) in the directory of the source code:
```shell
g++ aurora.cpp -o aurora -O3
```

You should now have a file named ```aurora.exe```, which is the engine. Now go to your GUI and find instructions for adding engines. When the GUI prompts you to choose a file/engine, choose ```aurora.exe```.

# Technical Details
Aurora uses a UCT (Upper Confidence Bounds applied to Trees) search with a NNUE (Efficiently Updatable Neural Network)

## Search
Here are some details about the search:
* Builds a search tree with nodes
* Nodes to expand are selected with UCT
* Values backpropagated up the tree are done so with minimax

## Evaluation
A NNUE trained with [bullet](https://github.com/jw1912/bullet) and options (the options are for ```vesta-8.nnue```):
* Arch:  (768->128)x2->1
* Scale: 400
* Epochs: 60
* WDL scheduler: constant 0.0
* LR scheduler: start 0.001 gamma 0.1 drop every 30 epochs
* Device: NVIDIA GeForce MX550
* Threads: 8
* Positions: 146688588
