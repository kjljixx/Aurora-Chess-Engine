# Aurora
![Aurora's logo, with background generated by DALL-E 2](https://github.com/kjljixx/Aurora-Chess-Engine/blob/main/Aurora.jpg)
<p style='text-align: right;'>Background image generated by DALL-E 2</em></p>

# General

Aurora is an actively developed UCI (Universal Chess Interface) compatible chess engine.
This means that Aurora is a command-line-based engine and you will need a UCI-Compatible GUI in order to intuitively use Aurora. See the "Usage" section below for usage instructions.

See [Playing Strength Testing](https://github.com/kjljixx/Aurora-Chess-Engine/wiki/Testing#playing-strength) for Aurora's playing strength.

## Usage

To use Aurora, simply download the source code and build ```aurora.cpp```.
For example, if you use g++, run the following command in the directory of the source code:
```shell
g++ aurora.cpp -o aurora -O3
```

You should now have a file named ```aurora.exe```, which is the engine. Now go to your GUI and find instructions for adding engines. When the GUI prompts you to choose a file/engine, choose ```aurora.exe```.

# Technical Details
Aurora uses a UCT (Upper Confidence Bounds applied to Trees) search with a HCE (Hand Crafted Evaluation)

## Search
Here are some details about the search:
* Builds a search tree with nodes
* Nodes to expand are selected with UCT
* Values backpropagated up the tree are done so with minimax

## Evaluation
Here are all the current evaluation terms in the HCE:
* Material Values
* Piece Square Tables
* Static Exchange Evaluation (While normally used for pruning in an alpha-beta engine, Aurora uses SEE for static board evaluation)
* Passed Pawn Bonuses
