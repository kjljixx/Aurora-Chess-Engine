#pragma once
#include "evaluation.h"
#include "tree.h"
#include <fstream>
#include <time.h>
#include <math.h>
#include <memory>
#include <chrono>
#include <deque>
#include <iomanip>

#if DATAGEN >= 1
  std::string dataFolderPath = "C:/Users/kjlji/OneDrive/Documents/VSCode/C++/AuroraChessEngine-main/data";
#endif

namespace search{

//parameters for search
enum backpropagationStrategy{AVERAGE, MINIMAX};
inline backpropagationStrategy backpropStrat = MINIMAX;

inline void init(){
  evaluation::init();
  zobrist::init();
  srand(time(NULL));
  std::cout.precision(10);
}

inline uint8_t selectEdge(Node* parent, bool isRoot){
  float maxPriority = -2;
  uint8_t maxPriorityNodeIndex = 0;

  const float parentVisitsTerm = (isRoot ? Aurora::rootExplorationFactor.value : Aurora::explorationFactor.value)*std::log(parent->visits)*std::sqrt(std::log(parent->visits));

  float varianceScale = 
    (1.0/parent->iters)*1.0+
    (1.0-1.0/parent->iters)*
    std::clamp<double>(
      1.0+Aurora::varianceScaleMultiplier.value*
            (std::sqrt(std::max(parent->variance(), float(0)))-Aurora::varianceScaleOffset.value),
      Aurora::varianceScaleMin.value,
      Aurora::varianceScaleMax.value
    );
  
  // std::cout << std::clamp(1.0+32*(std::sqrt(std::max(parent->variance(), float(0)))-0.00625), 0.2, 2.0) << " ";

  for(int i=0; i<parent->children.size(); i++){
    Node* currNode = parent->children[i].child;
    Edge currEdge = parent->children[i];

    //We can make a guess about how many visits a node had before it was pruned by LRU
    bool isLRUPruned = parent->children[i].edge.value & (1 << 15);

    float childVisits = currNode ? currNode->visits : 1;
    float boostTerm = 1.0 + Aurora::visitBoostMultiplier.value * (parent->visits * Aurora::visitBoostOffset.value) / 
                      (parent->visits * Aurora::visitBoostOffset.value + childVisits);

    float currPriority = -(currNode ? currNode->avgValue : currEdge.value)+
      boostTerm*
      varianceScale*
      parentVisitsTerm/std::sqrt(currNode ? currNode->visits : (isLRUPruned ? 14 : 1));

    assert(currPriority>=-1);

    if(currPriority>maxPriority){
      maxPriority = currPriority;
      maxPriorityNodeIndex = i;
    }
  }

  return maxPriorityNodeIndex;
}

inline void expand(Tree& tree, Node* parent, chess::MoveList& moves){
  if(moves.size()==0){return;}

  parent->children.resize(moves.size());
  tree.currSize += moves.size() * sizeof(Edge);

  for(uint16_t i=0; i<moves.size(); i++){
    parent->children[i] = Edge(moves[i]);
  }
}

int numTTHits = 0;
int numPlayouts = 0;

template<int numHiddenNeurons>
float playout(Tree& tree,chess::Board& board, evaluation::NNUE<numHiddenNeurons>& nnue){
  //First, check if position is terminal
  chess::gameStatus _gameStatus = chess::getGameStatus(board, chess::isLegalMoves(board));
  assert(-1<=_gameStatus && 2>=_gameStatus);
  if(_gameStatus != chess::ONGOING){
    return _gameStatus;
  }
  
  //Next, check TBs
  chess::gameStatus tbResult = chess::probeWdlTb(board);
  if(tbResult != chess::ONGOING){
    return tbResult;
  }
  
  numPlayouts++;
  //Next, check TT
  TTEntry* entry = tree.getTTEntry(board.history[board.halfmoveClock]);
  if(entry->hash == (board.history[board.halfmoveClock] >> 32) && entry->val != -2){
    numTTHits++;
    return entry->val;
  }

  //Next, do qSearch
  float eval = evaluation::cpToVal(evaluation::evaluate(board, nnue, tree.TT));
  entry->hash = (board.history[board.halfmoveClock] >> 32);
  entry->val = eval;

  assert(-1<=eval && 1>=eval);
  return eval;
}

inline void backpropagate(Tree& tree, float result, std::vector<std::pair<Edge*, U64>>& edges, uint8_t visits, bool forceResult, bool runFindBestMove, bool continueBackprop){
  //Backpropagate results
  if(edges.size() == 0){return;}

  std::pair<Edge*, U64> p = edges.back();
  Edge* currEdge = p.first; U64 hash = p.second;
  edges.pop_back();

  currEdge->child->visits += visits;

  float oldCurrNodeValue = 2;

  if(backpropStrat == AVERAGE){
    assert(0);
    // currEdge->value = (currEdge->value*currEdge->visits+result)/(currEdge->visits+1);
    // result = -result;
  }
  else if(backpropStrat == MINIMAX){
    //We only need to backpropagate two types of results here: the current best child becomes worse, or there is a new best child
    if(continueBackprop){
      //If currEdge is the best move and is backpropagated to become worse, we need to run findBestQ for the parent of currEdge
      oldCurrNodeValue = 2;
      if(currEdge->child->parent && edges.size() > 0 && -currEdge->value == edges.back().first->value){oldCurrNodeValue = currEdge->value;}

      //If the result is worse than the current value, there is no point in continuing the backpropagation, other than to add visits to the nodes
      if(result <= currEdge->value && !runFindBestMove && !forceResult){
        continueBackprop = false;

        currEdge->child->iters++;
        float newValWeight = std::clamp(1.0/currEdge->child->iters, double(Aurora::valSameMinWeight.value), 1.0);
        currEdge->child->avgValue = currEdge->child->avgValue*(1-newValWeight) + currEdge->value*newValWeight;
        currEdge->child->sumSquaredVals = currEdge->child->sumSquaredVals*(1-newValWeight) + currEdge->value*currEdge->value*newValWeight;

        TTEntry* entry = tree.getTTEntry(hash);
        entry->hash = hash >> 32;
        entry->val = currEdge->value;

        backpropagate(tree, result, edges, visits, false, runFindBestMove, continueBackprop);
        return;
      }

      currEdge->value = runFindBestMove ? -findBestQ(currEdge->child) : result;

      assert(-1<=currEdge->value && 1>=currEdge->value);

      runFindBestMove = currEdge->value > oldCurrNodeValue; //currEdge(which used to be the best child)'s value got worse from currEdge's parent's perspective

      result = -currEdge->value;

      currEdge->child->iters++;
      float newValWeight = std::clamp(1.0/currEdge->child->iters, double(Aurora::valChangedMinWeight.value), 1.0);
      currEdge->child->avgValue = currEdge->child->avgValue*(1-newValWeight) + currEdge->value*newValWeight;
      currEdge->child->sumSquaredVals = currEdge->child->sumSquaredVals*(1-newValWeight) + currEdge->value*currEdge->value*newValWeight;
    }
    else{
      currEdge->child->iters++;
      float newValWeight = std::clamp(1.0/currEdge->child->iters, double(Aurora::valSameMinWeight.value), 1.0);
      currEdge->child->avgValue = currEdge->child->avgValue*(1-newValWeight) + currEdge->value*newValWeight;
      currEdge->child->sumSquaredVals = currEdge->child->sumSquaredVals*(1-newValWeight) + currEdge->value*currEdge->value*newValWeight;
    }
  }

  TTEntry* entry = tree.getTTEntry(hash);
  entry->hash = hash >> 32;
  entry->val = currEdge->value;

  backpropagate(tree, result, edges, visits, false, runFindBestMove, continueBackprop);
}

inline void printSearchInfo(Tree& tree, std::chrono::steady_clock::time_point start, bool finalResult){
  std::cout << evaluation::numTTHits << "/" << evaluation::numQSearches << "\n";
  std::cout << numTTHits << "/" << numPlayouts << "\n";
  Node* root = tree.root;
  if(Aurora::outputLevel.value >= 3){
    std::cout << "NODES: " << root->visits;
    std::cout << " SELDEPTH: " << int(tree.seldepth) <<"\n";

    std::cout.precision(5);

    std::cout << std::left
              << std::setw(8) << "Move"
              << std::setw(12) << "Q"
              << std::setw(12) << "A"
              << std::setw(12) << "I"
              << std::setw(12) << "N"
              << std::setw(12) << "V"
              << "PV" << std::endl;

    std::cout << std::string(80, '-') << std::endl;

    std::vector<Edge> sortedEdges;
    for(const auto& edge : root->children) {
        sortedEdges.push_back(edge);
    }

    std::sort(sortedEdges.begin(), sortedEdges.end(), 
        [](const Edge& a, const Edge& b) {
            return a.value < b.value;
        });

    for(int i = 0; i < sortedEdges.size(); i++) {
        Edge currEdge = sortedEdges[i];

        std::cout << std::left
                  << std::setw(8) << currEdge.edge.toStringRep()
                  << std::setw(12) << -currEdge.value
                  << std::setw(12) << -(currEdge.child ? currEdge.child->avgValue : -2)
                  << std::setw(12) << (currEdge.child ? currEdge.child->iters : 0)
                  << std::setw(12) << (currEdge.child ? currEdge.child->visits : 1)
                  << std::setw(12) << (currEdge.child ? std::sqrt(currEdge.child->variance()) : -1);
        
        // Print PV sequence
        Node* pvNode = sortedEdges[i].child;
        while(pvNode && pvNode->children.size() > 0) {
            Edge pvEdge = findBestQEdge(pvNode);
            std::cout << pvEdge.edge.toStringRep() << " ";
            pvNode = pvEdge.child;
        }
        std::cout << std::endl;
    }

    std::cout.precision(10);
  }

  if(Aurora::outputLevel.value >= 2 || (finalResult && Aurora::outputLevel.value >= 1)){
    std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;

    std::cout <<
    "info depth " << (root->visits == tree.startNodes ? 0 : int(tree.depth / (root->visits - tree.startNodes))) <<
    " seldepth " << int(tree.seldepth) <<
    " nodes " << root->visits <<
    " score cp " << evaluation::valToCp(-findBestQ(root)) <<
    " hashfull " << int(tree.getHashfull()*1000) <<
    " nps " << std::round((root->visits-tree.previousVisits)/(elapsed.count()-tree.previousElapsed)) <<
    " time " << std::round(elapsed.count()*1000) <<
    " pv ";
    Node* pvNode = root;
    while(pvNode && pvNode->children.size() > 0){
      Edge pvEdge = findBestQEdge(pvNode);
      std::cout << pvEdge.edge.toStringRep() << " ";
      pvNode = pvEdge.child;
    }
    std::cout << std::endl;

    tree.previousVisits = root->visits; tree.previousElapsed = elapsed.count();
  }
}

//Code relating to the time manager
enum timeManagementType{
  FOREVER,
  TIME,
  NODES,
  ITERS
};

struct timeManagement{
  timeManagementType tmType = FOREVER;
  float hardLimit;
  float limit; //For FOREVER, this does not matter. For Nodes, this is the amount of nodes. For Time, it is the amount of seconds
  bool useSoftHardNodeLimits = false;
  timeManagement(timeManagementType _tmType, uint32_t _limit = 0): tmType(_tmType), hardLimit(_limit), limit(_limit) {}
  timeManagement(): tmType(FOREVER), hardLimit(0), limit(0){}
};

//The main search function
inline void search(chess::Board& rootBoard, timeManagement tm, Tree& tree){
  auto start = std::chrono::steady_clock::now();

  tree.setHash();
  if(Aurora::outputLevel.value >= 1){
    std::cout << "info string starting search with max tree size " <<
              (tree.sizeLimit == 0 ? "unlimited" : std::to_string(tree.sizeLimit/1000000.0)) << " mb "
              << "and TT size " <<
              (tree.TT.size()*sizeof(TTEntry)/1000000.0) << " mb"
              << std::endl;
    if(tree.TT.size() == 1){
      std::cout << "info string WARNING: TT is disabled, set either TTHash or Hash option to a non-zero value to enable" << std::endl;
    }
  }

  if(!tree.root){tree.push_back(Node()); tree.root = &tree.tree[tree.tree.size()-1];}

  tree.seldepth = 0;
  tree.depth = 0;

  evaluation::NNUE<NNUEhiddenNeurons> nnue(evaluation::_NNUEparameters);

  Node* currNode = tree.root;
  
  //For Printing Search Info
  int lastNodeCheck = 1;
  std::chrono::duration<float> elapsed = start - start;
  tree.previousVisits = tree.root->visits;
  tree.previousElapsed = 0;

  if(chess::getGameStatus(rootBoard, chess::isLegalMoves(rootBoard)) != chess::ONGOING){
    if(Aurora::outputLevel.value >= 0){
      std::cout << "bestmove a1a1" << std::endl;
    }
    return;
  }

  //For Time Management
  tree.startNodes = tree.root->visits;
  int bestMoveChanges = 0;
  float bestMoveChangesMultiplier = 1;
  chess::Move currBestMove;

  //First, Check TBs
  chess::Move tbMove = chess::probeDtzTb(rootBoard);
  if(tbMove.value){
    chess::gameStatus result = chess::probeWdlTb(rootBoard);
    chess::MoveList moves(rootBoard);
    expand(tree, tree.root, moves);
    for(int i=0; i<moves.size(); i++){
      if(moves[i] == tbMove){
        tree.root->children[i].value = -result+0.001;
      }
      else{
        tree.root->children[i].value = 1;
      }
    }
    tree.root->visits = 1;
    tm.tmType = NODES;
    tm.limit = -1;
  }

  while((tm.tmType == FOREVER) ||
        (tm.tmType == TIME &&
          ((tm.useSoftHardNodeLimits && elapsed.count()<std::min(tm.limit*bestMoveChangesMultiplier, tm.hardLimit)) ||
          (!tm.useSoftHardNodeLimits && elapsed.count()<tm.limit))
        ) ||
        (tm.tmType == NODES &&
          ((tm.useSoftHardNodeLimits && (tree.root->visits - tree.startNodes) < std::min(tm.limit*bestMoveChangesMultiplier, tm.hardLimit)) ||
          (!tm.useSoftHardNodeLimits && (tree.root->visits - tree.startNodes) < tm.limit))
        ) ||
        (tm.tmType == ITERS &&
          ((tm.useSoftHardNodeLimits && tree.root->iters < std::min(tm.limit*bestMoveChangesMultiplier, tm.hardLimit)) ||
          (!tm.useSoftHardNodeLimits && tree.root->iters < tm.limit))
        )
      ){
    chess::Board board = rootBoard;

    int currDepth = 0;
    currNode = tree.root; tree.moveToHead(tree.root);
    Edge* currEdge;
    std::vector<std::pair<Edge*, U64>> traversePath;

    //Traverse the search tree
    while(currNode->children.size() > 0){
      currDepth++;
      
      //Move all children nodes to the front of LRU
      for(int i=0; i<currNode->children.size(); i++){
        if(currNode->children[i].child != nullptr){
          tree.moveToHead(currNode->children[i].child);
        }
      }

      //Select Child Node to explore
      uint8_t currEdgeIndex = selectEdge(currNode, currNode == tree.root);

      currEdge = &currNode->children[currEdgeIndex];
      chess::makeMove(board, currEdge->edge);
      traversePath.push_back({currEdge, board.history[board.halfmoveClock]});

      //If we only had a child edge before, create the corresponding child node
      if(currEdge->child == nullptr){
        currEdge->child = tree.push_back(Node(currNode));
        currEdge->child->index = currEdgeIndex;
        currEdge->child->mark = currNode->mark;
        currEdge->child->visits = 1;
        currEdge->child->iters = 1;
        currEdge->child->avgValue = currEdge->value;
        currEdge->child->sumSquaredVals = currEdge->value*currEdge->value;
      }

      currNode = currEdge->child;
    }

    //Expand & Backpropagate new values
    if(currNode->isTerminal){
      tree.depth += currDepth;
      tree.root->visits += 1;
      tree.root->iters += 1;
      backpropagate(tree, currEdge->value, traversePath, 1, true, false, true);
    }
    else{
      //Reached a leaf node
      currDepth++;

      //Make sure game isn't terminal
      chess::MoveList moves(board);
      if(chess::getGameStatus(board, moves.size()!=0) != chess::ONGOING){
        assert(currEdge->value>=-1);
        currNode->isTerminal=true;
        continue;
      }

      //Create new child edges
      expand(tree, currNode, moves);

      //Get values for all created edges
      Node* parentNode = currNode; //This will be where the backpropagation starts

      float currBestValue = 2;

      nnue.refreshAccumulator(board);
      std::array<std::array<int16_t, NNUEhiddenNeurons>, 2> currAccumulator = nnue.accumulator;

      for(int i=0; i<parentNode->children.size(); i++){
        currEdge = &parentNode->children[i];

        chess::Board movedBoard = board;

        nnue.accumulator = currAccumulator;
        nnue.updateAccumulator(movedBoard, currEdge->edge);

        currEdge->value = playout(tree, movedBoard, nnue);
        assert(-1<=currEdge->value && 1>=currEdge->value);
        
        currBestValue = std::min(currBestValue, currEdge->value);
      }

      int visits = 0;
      for(int i=0; i<parentNode->children.size(); i++){
        if(parentNode->children[i].value <= currBestValue + Aurora::visitWindow.value){
          visits++;
        }
      }
      assert(visits >= 1);

      tree.depth += currDepth*visits;

      //Update root stats, since backpropagation doesn't reach the root
      tree.root->visits += visits;
      tree.root->iters += 1;

      //Backpropagate best value
      backpropagate(tree, -currBestValue, traversePath, visits, true, false, true);
    }

    if(currDepth > tree.seldepth){tree.seldepth = currDepth;}

    //Output some information on the search occasionally
    elapsed = std::chrono::steady_clock::now() - start;
    if(elapsed.count() >= lastNodeCheck*2){
      lastNodeCheck++;
      printSearchInfo(tree, start, false);
    }

    //Decide if we want to search longer or shorter depending on how much the best move has changed
    if(tm.useSoftHardNodeLimits){
      if(findBestQEdge(tree.root).edge.value != currBestMove.value){
        bestMoveChanges++;
        currBestMove = findBestQEdge(tree.root).edge;
      }

    double expectedBestMoveChanges =
      Aurora::bestMoveChangesCoefficient.value *
      (std::pow(tree.root->visits, Aurora::bestMoveChangesExponent.value) -
       std::pow(tree.startNodes, Aurora::bestMoveChangesExponent.value));
    const double bestMoveChangesMultiplierMin =
      std::min(double(Aurora::bestMoveChangesMultiplierMin.value),
              double(Aurora::bestMoveChangesMultiplierMax.value));
    const double bestMoveChangesMultiplierMax =
      std::max(double(Aurora::bestMoveChangesMultiplierMin.value),
              double(Aurora::bestMoveChangesMultiplierMax.value));
    bestMoveChangesMultiplier =
      std::clamp(bestMoveChanges / expectedBestMoveChanges,
                bestMoveChangesMultiplierMin,
                bestMoveChangesMultiplierMax);
    }
  }

  //Output the final result of the search
  printSearchInfo(tree, start, true);
  if(Aurora::outputLevel.value >= 0){
    std::cout << "\nbestmove " << findBestAEdge(tree.root).edge.toStringRep() << std::endl;
  }

  return;
}

//Same as chess::makeMove except we move the root so we can keep nodes from an earlier search
//Parameter "board" must be different than parameter "rootBoard"
inline void makeMove(chess::Board& board, chess::Move move, chess::Board& rootBoard, Tree& tree){
  if(tree.root == nullptr ||
    board.equivalentHistory(rootBoard) == false
  ){
      chess::makeMove(board, move);
      return;
  }

  chess::makeMove(board, move);

  Edge newRootEdge = Edge(chess::Move());
  for(int i=0; i<tree.root->children.size(); i++){
    if(tree.root->children[i].edge == move){
      newRootEdge = tree.root->children[i];
      break;
    }
  }
  Node* newRoot = newRootEdge.child;

  if(newRoot == nullptr){tree.root = nullptr; destroyTree(tree); return;}

  tree.root = moveRootToChild(tree, newRoot, tree.root);

  tree.root->parent = nullptr;
  tree.root->visits--;//Visits needs to be subtracted by 1 to remove the visit which added the node
  tree.root->iters--;//Same logic for iters

  chess::makeMove(rootBoard, move);
}

}//namespace search