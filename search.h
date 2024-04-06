#pragma once
#include "evaluation.h"
#include <fstream>
#include <time.h>
#include <math.h>
#include <memory>
#include <chrono>
#include <deque>

#if DATAGEN >= 1
  std::string dataFolderPath = "C:/Users/kjlji/OneDrive/Documents/VSCode/C++/AuroraChessEngine-main/data";
#endif

namespace search{

//parameters for search
enum backpropagationStrategy{AVERAGE, MINIMAX}; //AVERAGE no longer works
backpropagationStrategy backpropStrat = MINIMAX;

void init(){
  Aurora::initOptions();
  evaluation::init();
  zobrist::init();
  srand(time(NULL));
  std::cout.precision(10);
}

struct Node;

struct Edge{
  Node* child;
  chess::Move move;

  Edge(Node* child, chess::Move move) : child(child), move(move) {}
};

struct Node{
  std::vector<Edge> children;
  uint32_t visits;
  float value;
  bool isTerminal;

  //For Tree Reuse
  Node* newAddress = nullptr;
  bool mark = false; //Also used in tree traversal

  Node() : visits(0), value(-2), isTerminal(false) {}
};

struct Test{
  std::vector<Edge> children;
  uint32_t info;
  float value;
  // bool isTerminal;

  //For Tree Reuse
  Test* newAddress = nullptr;
  // bool mark = false; //Also used in tree traversal

  bool mark(){
    return info & 2;
  }
  bool isTerminal(){
    return info & 1;
  }
  uint32_t visits(){
    return info >> 2;
  }
};

struct TTEntry{
  Node* node;
  U64 hash;

  TTEntry() : node(nullptr), hash(0) {}
  TTEntry(Node* node, U64 hash) : node(node), hash(hash) {}
};

const int TT_DEFAULT_SIZE = 4194304;

struct TT{
  std::vector<TTEntry> tt;
  uint32_t size;
  U64 mask;

  void init(uint32_t _size){
    size = _size;
    mask = _size - 1;
    tt.resize(_size, TTEntry());
    std::fill(tt.begin(), tt.end(), TTEntry());
  }

  void storeEntry(TTEntry entry){
    tt[entry.hash & mask] = entry;
  }

  TTEntry getEntry(U64 hash){
    return tt[hash & mask];
  }
};

struct Tree{
  std::deque<Node> tree;
  TT tt;

  Tree(){
    tt.init(TT_DEFAULT_SIZE);
  }
};

void destroyTree(Tree& tree){
  tree.tree.clear();
  tree.tt.init(TT_DEFAULT_SIZE);
}

uint64_t markSubtree(Node* node, bool isSubtreeRoot = true, bool unmarked = true){
  uint64_t markedNodes = 0;

  if(isSubtreeRoot){
    unmarked = node->mark;
  }
  if(node){
    for(Edge edge : node->children){
      markedNodes += markSubtree(edge.child, false, unmarked);
    }
    node->mark = !unmarked;
    markedNodes++;
  }
  return markedNodes;
}

//Returns a pointer to the new root, which is different from the pointer given as a parameter because of the garbage collection
Node* moveRootToChild(Tree& tree, Node* newRoot, Node* currRoot){
  //LISP 2 Garbage Collection Algorithm (https://en.wikipedia.org/wiki/Mark%E2%80%93compact_algorithm#LISP_2_algorithm)
  uint64_t markedNodes = markSubtree(newRoot);
  bool marked = newRoot->mark;

  uint64_t freePointer = 0;

  for(uint32_t i=0; i<tree.tree.size(); i++){
    Node* livePointer = &tree.tree[i];

    if(livePointer->mark == marked){
      livePointer->newAddress = &tree.tree[freePointer];
      freePointer++;
    }
  }

  Node* newRootNewAddress = newRoot->newAddress;

  for(Node& node : tree.tree){
    if(node.mark == marked){
      for(int i=0; i<node.children.size(); i++){
        node.children[i].child = node.children[i].child->newAddress;
      }
    }
  }

  for(uint32_t i=0; i<tree.tree.size(); i++){
    Node* livePointer = &tree.tree[i];
    if(livePointer->mark == marked){
      *(livePointer->newAddress) = *livePointer;
      assert(tree.tree[i].value == tree.tree[i].newAddress->value);
    }
    livePointer++;
  }

  tree.tree.resize(markedNodes);

  return newRootNewAddress;
}

Edge selectEdge(Node* parent, bool isRoot){
  float maxPriority = -2;
  uint8_t maxPriorityNodeIndex = 0;

  const float parentVisitsTerm = (isRoot ? Aurora::options["rootExplorationFactor"].value : Aurora::options["explorationFactor"].value)*std::sqrt(std::log(parent->visits));

  for(int i=0; i<parent->children.size(); i++){
    Node* currNode = parent->children[i].child;

    float currPriority = -currNode->value+parentVisitsTerm/std::sqrt(currNode->visits);

    assert(currPriority>=-1);

    if(currPriority>maxPriority){
      maxPriority = currPriority;
      maxPriorityNodeIndex = i;
    }
  }

  return parent->children[maxPriorityNodeIndex];
}

void expand(Tree& tree, Node* parent, chess::Board& board, chess::MoveList& moves){
  if(moves.size()==0){return;}

  Node* currNode;

  for(uint16_t i=0; i<moves.size(); i++){
    tree.tree.push_back(Node());
    currNode = &tree.tree.back();
    parent->children.push_back(Edge(currNode, moves[i]));
    // U64 hash = zobrist::updateHash(board, moves[i]);
    // if(tree.tt.getEntry(hash).hash == 0){
    //   tree.tt.storeEntry(TTEntry(currNode, hash));
    // }
    currNode->mark = parent->mark;
  }
}

float playout(chess::Board& board, Node* currNode, evaluation::NNUE& nnue){
  chess::gameStatus _gameStatus = chess::getGameStatus(board, chess::isLegalMoves(board));
  assert(-1<=_gameStatus && 2>=_gameStatus);
  if(_gameStatus != chess::ONGOING){
    currNode->isTerminal = true;
    if(_gameStatus == chess::LOSS){return float(_gameStatus);}
    return _gameStatus;
  }

  float eval = std::max(std::min(std::atan(evaluation::evaluate(board, nnue)*Aurora::options["evalScaleFactor"].value/100.0)/1.57079633, 1.0),-1.0)*0.999999;
  assert(-1<=eval && 1>=eval);
  return eval;
}

Edge findBestEdge(Node* parent){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
  Edge currBestMove = parent->children[0];

  Node* currNode;
  for(int i=0; i<parent->children.size(); i++){
    currNode = parent->children[i].child;
    if(currNode->value < currBestValue){
      currBestValue = currNode->value;
      currBestMove = parent->children[i];
    }
  }

  return currBestMove;
}

float findBestValue(Node* parent){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective

  Node* currNode;
  for(int i=0; i<parent->children.size(); i++){
    currNode = parent->children[i].child;
    currBestValue = std::min(currBestValue, currNode->value);
  }

  return currBestValue;
}

int previousVisits = 0;
int previousElapsed = 0;

void printSearchInfo(Node* root, std::chrono::steady_clock::time_point start, bool finalResult){
  if(Aurora::options["outputLevel"].value==3){
    std::cout << "NODES: " << root->visits;

    for(int i=0; i<root->children.size(); i++){
      Edge currEdge = root->children[i];
      std::cout << currEdge.move.toStringRep() << ": Q:" << -currEdge.child->value << " N:" << currEdge.child->visits << " PV:";
      Node* pvNode = currEdge.child;
      while(pvNode->children.size() > 0){
        Edge pvEdge = findBestEdge(pvNode);
        std::cout << pvEdge.move.toStringRep() << " ";
        pvNode = pvEdge.child;
      }
      std::cout << std::endl;
    }
  }

  if(Aurora::options["outputLevel"].value >= 2 || (finalResult && Aurora::options["outputLevel"].value >= 1)){
    std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;

    std::cout <<
      "info nodes " << root->visits <<
          " score cp " << round(tan(-findBestValue(root)*1.57079633)*100) <<
          " nps " << round((root->visits-previousVisits)/(elapsed.count()-previousElapsed)) <<
          " time " << round(elapsed.count()*1000) <<
          " pv ";
    Node* pvNode = root;
    while(pvNode->children.size() > 0){
      Edge pvEdge = findBestEdge(pvNode);
      std::cout << pvEdge.move.toStringRep() << " ";
      pvNode = pvEdge.child;
    }
    std::cout << std::endl;

    previousVisits = root->visits; previousElapsed = elapsed.count();
  }
}

/*
Params:
result: the result to backpropagate
traversePath: the path to backpropagate on
visits: the amount of visits to add to each node as we backpropagate
runFindBestMove: whether or not to do a re-check of all child nodes to find the best value, or just use result (main purpose is to be utilized by the function as it does recursion)
continueBackprop: whether or not to continue to backpropagate the result (main purpose is to be utilized by the function as it does recursion)
forceResult: whether or not to force the currNode to take the value of result (normally, if the result is worse than the current value of the node, we will not set the value of the node to the result)
*/
void backpropagate(float result, std::vector<Node*>& traversePath, uint8_t visits, bool runFindBestMove, bool continueBackprop, bool forceResult){
  //Backpropagate results
  Node* currNode = traversePath.back(); traversePath.pop_back();

  if(currNode == nullptr){return;}

  currNode->visits+=visits;

  //If currNode is the best move and is backpropagated to become worse, we need to run findBestValue for the parent of currNode
  float oldCurrNodeValue = currNode->value;

  if(backpropStrat == AVERAGE){
    currNode->value = (currNode->value*currNode->visits+result)/(currNode->visits+1);
    result = -result;
  }
  else if(backpropStrat == MINIMAX){
    //We only need to backpropagate two types of results here: the current best child becomes worse, or there is a new best child
    if(continueBackprop){

      //If the result is worse than the current value, there is no point in continuing the backpropagation, other than to add visits to the nodes
      if(result <= currNode->value && !runFindBestMove && !forceResult){
        continueBackprop = false;
      }
      else{
        currNode->value = runFindBestMove ? -findBestValue(currNode) : result;
        assert(-1<=currNode->value && 1>=currNode->value);
      }
    }
    if(std::ssize(traversePath) > 0){
      Node* parent = traversePath.back();
      bool _runFindBestMove = -oldCurrNodeValue == parent->value && currNode->value > oldCurrNodeValue; //currNode(which used to be the best child)'s value got worse from currNode's parent's perspective
      backpropagate(-currNode->value, traversePath, visits, _runFindBestMove, continueBackprop, false);
    }
  }
}

//Code relating to the time manager
enum timeManagementType{
  FOREVER,
  TIME,
  NODES
};

struct timeManagement{
  timeManagementType tmType = FOREVER;
  float limit; //For FOREVER, this does not matter. For Nodes, this is the amount of nodes. For Time, it is the amount of seconds
  timeManagement(timeManagementType _tmType, uint32_t _limit = 0): tmType(_tmType), limit(_limit) {}
  timeManagement(): tmType(FOREVER), limit(0){}
};

//The main search function
Node* search(chess::Board& rootBoard, timeManagement tm, Node* root, Tree& tree){
  auto start = std::chrono::steady_clock::now();

  if(!root){tree.tree.push_back(Node()); root = &tree.tree.back();}

  evaluation::NNUE nnue;

  Node* currNode = root;
  
  int lastNodeCheck = 1;
  std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;
  previousVisits = root->visits;
  previousElapsed = 0;

  if(chess::getGameStatus(rootBoard, chess::isLegalMoves(rootBoard)) != chess::ONGOING){
    #if DATAGEN != 1
      std::cout << "bestmove a1a1" << std::endl;
    #endif
    return root;
  }

  while((tm.tmType == FOREVER) || (elapsed.count()<tm.limit && tm.tmType == TIME) || (root->visits<tm.limit && tm.tmType == NODES)){
    currNode = root;
    chess::Board board = rootBoard;

    //Traverse the search tree
    std::vector<Node*> traversePath;
    traversePath.push_back(currNode);
    while(currNode->children.size() > 0){
      Edge currEdge = selectEdge(currNode, currNode == root);

      chess::makeMove(board, currEdge.move);

      currNode = currEdge.child;
      traversePath.push_back(currNode);
    }

    //Expand & Backpropagate new values
    if(currNode->isTerminal){
      backpropagate(currNode->value, traversePath, 1, false, true, true);
    }
    else{//Reached a leaf node
      //Create new child nodes
      chess::MoveList moves(board);
      if(chess::getGameStatus(board, moves.size()!=0) != chess::ONGOING){assert(currNode->value>=-1); currNode->isTerminal=true; continue;}
      expand(tree, currNode, board, moves);

      //Simulate for all new nodes
      Node* parentNode = currNode; //This will be the root of the backpropagation

      float currBestValue = 2; //Find and only backpropagate the best value we end up finding

      nnue.refreshAccumulator(board);
      std::array<std::array<int16_t, NNUEhiddenNeurons>, 2> currAccumulator = nnue.accumulator;

      for(int i=0; i<parentNode->children.size(); i++){
        Edge currEdge = parentNode->children[i];
        currNode = currEdge.child;

        chess::Board movedBoard = board;
        nnue.accumulator = currAccumulator;

        nnue.updateAccumulatorAndMakeMove(movedBoard, currEdge.move);
        float result;
        result = playout(movedBoard, currNode, nnue);
        assert(-1<=result && 1>=result);

        currNode->value = result;
        currNode->visits = 1;

        currBestValue = fminf(currBestValue, result);
      }

      //Backpropagate best value
      backpropagate(-currBestValue, traversePath, 1, false, true, true);
    }

    //Output some information on the search occasionally
    elapsed = std::chrono::steady_clock::now() - start;
    #if DATAGEN != 1
      if(elapsed.count() >= lastNodeCheck*2){
        lastNodeCheck++;
        printSearchInfo(root, start, false);
      }
    #endif
  }
  
  //Output the final result of the search
  #if DATAGEN != 1
    printSearchInfo(root, start, true);
    std::cout << "\nbestmove " << findBestEdge(root).move.toStringRep() << std::endl;
  #endif

  return root;
}
//Same as chess::makeMove except we move the root so we can keep nodes from an earlier search
//Parameter "board" must be different than parameter "rootBoard"
void makeMove(chess::Board& board, chess::Move move, chess::Board& rootBoard, Node*& root, Tree& tree){
  if(root == nullptr || zobrist::getHash(board) != zobrist::getHash(rootBoard)){chess::makeMove(board, move); return;}

  chess::makeMove(board, move);

  Node* newRoot = nullptr;
  for(int i=0; i<root->children.size(); i++){
    Edge currEdge = root->children[i];
    if(currEdge.move == move){newRoot = currEdge.child; break;}
  }

  if(newRoot == nullptr){root = nullptr; destroyTree(tree); return;}

  root = moveRootToChild(tree, newRoot, root);

  root->visits--;//Visits needs to be subtracted by 1 to remove the visit which added the node

  chess::makeMove(rootBoard, move);
}

}//namespace search