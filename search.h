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

#if DATAGEN == 0
uint8_t seldepth = 0;
#endif

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
  uint8_t depth;
  float sPriority;
  bool updatePriority;

  //For Tree Reuse
  Node* newAddress = nullptr;
  bool mark = false; //Also used in tree traversal

  Node(uint8_t depth) :
  visits(0), value(-2), isTerminal(false), depth(depth), sPriority(-1), updatePriority(true) {}

  Node() : visits(0), value(-2), isTerminal(false), depth(0), sPriority(-1), updatePriority(true) {}
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
    node->mark = !unmarked;
    markedNodes++;
    for(Edge edge : node->children){
      markedNodes += markSubtree(edge.child, false, unmarked);
    }
  }
  return markedNodes;
}

//Returns a pointer to the new root, which is different from the pointer given as a parameter because of the garbage collection
Node* moveRootToChild(Tree& tree, Node* newRoot){
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
      for(int i=0; i<std::ssize(node.children); i++){
        assert(node.children[i].child->mark == marked);
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

  const float parentVisitsTerm = (isRoot ? Aurora::options["rootExplorationFactor"].value
                                         : Aurora::options["explorationFactor"].value) * std::sqrt(std::log(parent->visits));

  for(int i=0; i<std::ssize(parent->children); i++){
    Node* currNode = parent->children[i].child;
    
    currNode->sPriority = -currNode->value+parentVisitsTerm/std::sqrt(currNode->visits);
    currNode->updatePriority = false;
      
    float currPriority = currNode->sPriority;

    assert(currPriority>=-1);

    if(currPriority>maxPriority){
      maxPriority = currPriority;
      maxPriorityNodeIndex = i;
    }
  }

  return parent->children[maxPriorityNodeIndex];
}

void expand(Tree& tree, Node* parent, chess::Board& board, chess::MoveList& moves, bool originalMark){
  if(moves.size()==0){return;}

  Node* currNode;

  for(uint16_t i=0; i<moves.size(); i++){
    tree.tree.push_back(Node(parent->depth+1));
    currNode = &tree.tree[tree.tree.size()-1];
    parent->children.push_back(Edge(currNode, moves[i]));
    
    U64 hash = zobrist::updateHash(board, moves[i]);
    if(tree.tt.getEntry(hash).hash == 0){
      tree.tt.storeEntry(TTEntry(currNode, hash));
    }
    
    currNode->mark = originalMark;
  }

  #if DATAGEN == 0
  if(parent->depth+1>seldepth){seldepth = parent->depth+1;}
  #endif
}

float playout(chess::Board& board, Node* currNode, evaluation::NNUE& nnue){
  chess::gameStatus _gameStatus = chess::getGameStatus(board, chess::isLegalMoves(board), true);
  assert(-1<=_gameStatus && 2>=_gameStatus);
  if(_gameStatus != chess::ONGOING){
    currNode->isTerminal = true;
    if(_gameStatus == chess::LOSS){return double(_gameStatus)+0.00000001*currNode->depth;}
    return _gameStatus;
  }

  float eval = std::max(std::min(std::atan(evaluation::evaluate(board, nnue)*Aurora::options["evalScaleFactor"].value/100.0)/1.57079633, 1.0),-1.0)*0.999999;
  assert(-1<=eval && 1>=eval);
  return eval;
}

Edge findBestEdge(Node* parent){
  assert(parent->children.size() > 0);

  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
  Edge currBestMove = parent->children[0];

  Node* currNode;
  for(int i=0; i<std::ssize(parent->children); i++){
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
  for(int i=0; i<std::ssize(parent->children); i++){
    currNode = parent->children[i].child;
    currBestValue = std::min(currBestValue, currNode->value);
  }

  return currBestValue;
}

int previousVisits = 0;
int previousElapsed = 0;

void printSearchInfo(Node* root, chess::Board& rootBoard, std::chrono::steady_clock::time_point start, bool finalResult){
  if(Aurora::options["outputLevel"].value==3){
    std::cout << "NODES: " << root->visits;

    #if DATAGEN == 0
    std::cout << " SELDEPTH: " << seldepth-root->depth << std::endl;
    #endif

    for(int i=0; i<std::ssize(root->children); i++){
      Edge currEdge = root->children[i];
      std::cout << currEdge.move.toStringRep() << ": Q:" << -currEdge.child->value << " N:" << currEdge.child->visits << " SP:" << currEdge.child->sPriority <<  " PV:";
      
      Node* pvNode = currEdge.child;
      chess::Board pvBoard = rootBoard; chess::makeMove(pvBoard, currEdge.move);
      while(pvNode->children.size() > 0){
        Edge pvEdge = findBestEdge(pvNode);
        std::cout << pvEdge.move.toStringRep() << " ";

        chess::makeMove(pvBoard, pvEdge.move);
        if(chess::countRepetitions(pvBoard) >= 2){break;}
        
        pvNode = pvEdge.child;
      }
      std::cout << std::endl;
    }
  }

  if(Aurora::options["outputLevel"].value >= 2 || (finalResult && Aurora::options["outputLevel"].value >= 1)){
    std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;

    std::cout << "info ";
      #if DATAGEN == 0
      std::cout << "depth " << seldepth-root->depth;
      #endif
      std::cout << " nodes " << root->visits <<
      " score cp " << round(tan(-findBestValue(root)*1.57079633)*100) <<
      " nps " << round((root->visits-previousVisits)/(elapsed.count()-previousElapsed)) <<
      " time " << round(elapsed.count()*1000) <<
      " pv ";
    Node* pvNode = root;
    chess::Board pvBoard = rootBoard;
    while(pvNode->children.size() > 0){
      Edge pvEdge = findBestEdge(pvNode);
      std::cout << pvEdge.move.toStringRep() << " ";
      
      chess::makeMove(pvBoard, pvEdge.move);
      if(chess::countRepetitions(pvBoard) >= 2){break;}
      
      pvNode = pvEdge.child;
    }
    std::cout << std::endl;

    previousVisits = root->visits; previousElapsed = elapsed.count();
  }
}

/*
Params:
result: the result to backpropagate
currNode: the node from which to start the backpropagation (including the node itself)
visits: the amount of visits to add to each node as we backpropagate
runFindBestMove: whether or not to do a re-check of all child nodes to find the best value, or just use result (main purpose is to be utilized by the function as it does recursion)
continueBackprop: whether or not to continue to backpropagate the result (main purpose is to be utilized by the function as it does recursion)
forceResult: whether or not to force the currNode to take the value of result (normally, if the result is worse than the current value of the node, we will not set the value of the node to the result)
originalMark: the 'mark' of the tree. All nodes except the nodes traversed by the search in this iteration should have node.mark = originalMark
onlyBackpropMarked: Only backpropagate through nodes which are marked (!originalMark). If onlyBackpropMarked is false, it is assumed (and necessary) that the entire tree has a mark of originalMark
*/
void backpropagate(float result, std::vector<Node*>& traversePath, uint8_t visits, bool runFindBestMove, bool continueBackprop, bool forceResult, bool originalMark, bool onlyBackpropMarked){
  //Backpropagate results
  Node* currNode = traversePath[traversePath.size()-1];

  if(currNode == nullptr){return;}

  currNode->visits+=visits;
  currNode->updatePriority = true;
  currNode->mark = originalMark;

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
    if(int(traversePath.size())-2 >= 0){
      Node* parent = traversePath[traversePath.size()-2];
      bool _runFindBestMove = -oldCurrNodeValue == parent->value && currNode->value > oldCurrNodeValue; //currNode(which used to be the best child)'s value got worse from currNode's parent's perspective
      traversePath.pop_back();
      backpropagate(-currNode->value, traversePath, visits, _runFindBestMove, continueBackprop, false, originalMark, onlyBackpropMarked);
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

  if(!root){tree.tree.push_back(Node()); root = &tree.tree[tree.tree.size()-1];}

  #if DATAGEN == 0
  seldepth = 0;
  #endif

  evaluation::NNUE nnue;

  Node* currNode = root;
  
  int lastNodeCheck = 1;
  std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;
  previousVisits = root->visits;
  previousElapsed = 0;

  if(chess::getGameStatus(rootBoard, chess::isLegalMoves(rootBoard), true) != chess::ONGOING){
    #if DATAGEN != 1
      std::cout << "bestmove a1a1\n";
    #endif
    return root;
  }

  while((tm.tmType == FOREVER) || (elapsed.count()<tm.limit && tm.tmType == TIME) || (root->visits<tm.limit && tm.tmType == NODES)){
    currNode = root;
    chess::Board board = rootBoard;
    //Traverse the search tree
    bool originalMark = root->mark; //Used to detect when we have traversed a cycle
    std::vector<Node*> traversed; traversed.push_back(currNode);
    while(currNode->children.size() > 0){
      Edge currEdge = selectEdge(currNode, currNode == root);
      
      chess::makeMove(board, currEdge.move);
      currNode = currEdge.child;
      
      traversed.push_back(currNode);
    }
    //Expand & Backpropagate new values
    if(currNode->isTerminal || currNode->mark != originalMark){
      backpropagate(currNode->value, traversed, 1, false, true, true, originalMark, true);
    }
    else{
      //Reached a leaf node
      chess::MoveList moves(board);
      if(chess::getGameStatus(board, moves.size()!=0, true) != chess::ONGOING){assert(currNode->value>=-1); currNode->isTerminal=true; continue;}
      //Create new child nodes
      expand(tree, currNode, board, moves, originalMark);
      
      //Simulate for all new nodes
      Node* parentNode = currNode; //This will be the root of the backpropagation
      
      float currBestValue = 2; //Find and only backpropagate the best value we end up finding
      
      nnue.refreshAccumulator(board);
      std::array<std::array<int16_t, NNUEhiddenNeurons>, 2> currAccumulator = nnue.accumulator;
      
      for(int i=0; i<std::ssize(parentNode->children); i++){
        Edge currEdge = parentNode->children[i];
        currNode = currEdge.child;

        U64 hash = zobrist::updateHash(board, currEdge.move);
        TTEntry entry = tree.tt.getEntry(hash);

        float result;
        
        if(entry.hash != hash || entry.node == currNode){
          chess::Board movedBoard = board;
          nnue.accumulator = currAccumulator;

          nnue.updateAccumulator(movedBoard, currEdge.move);
          result = playout(movedBoard, currNode, nnue);
        }
        else{
          result = entry.node->value;
        }
        assert(-1<=result && 1>=result);
        
        currNode->value = result;
        currNode->visits = 1;
        currNode->updatePriority = true;

        currBestValue = fminf(currBestValue, result);
      }
      //Backpropagate best value
      backpropagate(-currBestValue, traversed, 1, false, true, true, originalMark, true);
    }
    //Output some information on the search occasionally
    elapsed = std::chrono::steady_clock::now() - start;
    #if DATAGEN != 1
      if(elapsed.count() >= lastNodeCheck*2){
        lastNodeCheck++;
        printSearchInfo(root, rootBoard, start, false);
      }
    #endif
  }
  //Output the final result of the search
  #if DATAGEN != 1
    printSearchInfo(root, rootBoard, start, true);
    std::cout << "\nbestmove " << findBestEdge(root).move.toStringRep() << "\n";
  #endif

  return root;
}
//Same as chess::makeMove except we move the root so we can keep nodes from an earlier search
//Parameter "board" must be different than parameter "rootBoard"
void makeMove(chess::Board& board, chess::Move move, chess::Board& rootBoard, Node*& root, Tree& tree){
  if(root == nullptr || zobrist::getHash(board) != zobrist::getHash(rootBoard)){chess::makeMove(board, move); return;}

  chess::makeMove(board, move);

  Node* newRoot = nullptr;
  for(int i=0; i<std::ssize(root->children); i++){
    Edge currEdge = root->children[i];
    if(currEdge.move == move){newRoot = currEdge.child; break;}
  }

  if(newRoot == nullptr){root = nullptr; destroyTree(tree); return;}

  root = moveRootToChild(tree, newRoot);

  root->visits--;//Visits needs to be subtracted by 1 to remove the visit which added the node

  chess::makeMove(rootBoard, move);
}

}//namespace search
