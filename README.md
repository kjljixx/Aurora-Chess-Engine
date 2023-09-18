# Aurora
Aurora is a work-in-progress UCI (Universal Chess Interface) compatible chess engine.
This means that Aurora is a command-line-based engine and you will need a UCI-Compatible GUI in order to intuitively use Aurora.

To use Aurora, simply download the source code and build ```aurora.cpp```.
For example, if you use g++, run the following command in the directory of the source code:
```shell
g++ aurora.cpp -o aurora
```

You should now have a file named ```aurora.exe```, which is the engine. Now go to your GUI and find instructions for adding engines. When the GUI prompts you to choose a file/engine, choose ```aurora.exe```.

Based on tests against Shallow Blue v2.0.0 (1713 CCRL Elo), Aurora is about ~1720 elo.

# Technical Details
Aurora uses a UCT (Upper Confidence Bounds applied to Trees) search with a HCE (Hand Crafted Evaluation)

# Search
Here are some details about the search:
* Builds a search tree with nodes
* Nodes to expand are selected with UCT
* Values backpropagated up the tree are done so with minimax

# Evaluation
Here are all the current evaluation terms in the HCE:
* Material Values
* Piece Square Tables
* Static Exchange Evaluation (While normally used for pruning in an alpha-beta engine, Aurora uses SEE for static board evaluation)
* Passed Pawn Bonuses
