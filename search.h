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
enum backpropagationStrategy{AVERAGE, MINIMAX};
backpropagationStrategy backpropStrat = MINIMAX;

//float eP[12] = {1.6301532566281178, 0.3415019889631426, 1.462459315326555, 0.09830134955092976, 0.3670339438501686, 0.5028838849947221, 0.28917477475978387, 1.581231015213, 0.2747746463404976, 0.9214915071600298, 0.14796697203232123, 1.2260899419271722}; //exploration parameters

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
  chess::Move edge;
  float value;

  Edge() : child(nullptr), edge(chess::Move()), value(-2) {}
  Edge(chess::Move move) : child(nullptr), edge(move), value(-2) {}
};

struct Node{
  Node* parent;
  uint8_t index;
  std::vector<Edge> children;
  uint32_t visits;
  bool isTerminal;
  uint8_t depth;
  float sPriority;
  bool updatePriority;

  //For Tree Reuse
  Node* newAddress = nullptr;
  bool mark = false;

  Node(Node* parent, uint8_t index, uint8_t depth) :
  parent(parent), index(index),
  visits(0), isTerminal(false), depth(depth), sPriority(-1), updatePriority(true) {}

  Node() : parent(nullptr), index(0), visits(0), isTerminal(false), depth(0), sPriority(-1), updatePriority(true) {}
};

struct Tree{
  std::deque<Node> tree;
};

void destroyTree(Tree& tree){
  tree.tree.clear();
}

uint64_t markSubtree(Node* node, bool isSubtreeRoot = true, bool unmarked = true){
  uint64_t markedNodes = 0;

  if(isSubtreeRoot){
    unmarked = node->mark;
  }
  if(node){
    node->mark = !unmarked;
    markedNodes++;
    for(int i=0; i<node->children.size(); i++){
      markedNodes += markSubtree(node->children[i].child, false, unmarked);
    }
  }
  return markedNodes;
}

//Returns a pointer to the new root, which is different from the pointer given as a parameter because of the garbage collection
Node* moveRootToChild(Tree& tree, Node* newRoot, Node* currRoot){
  //LISP 2 Garbage Collection Algorithm (https://en.wikipedia.org/wiki/Mark%E2%80%93compact_algorithm#LISP_2_algorithm)
  uint64_t markedNodes = markSubtree(newRoot);
  bool marked = newRoot->mark;

  //std::cout << "\n" << currRoot << ":" << currRoot->mark << " " << newRoot << ":" << newRoot->mark << " | ";

  uint64_t freePointer = 0;

  for(uint32_t i=0; i<tree.tree.size(); i++){
    Node* livePointer = &tree.tree[i];
    //std::cout << livePointer << ":" << livePointer->mark << " ";
    if(livePointer->mark == marked){
      livePointer->newAddress = &tree.tree[freePointer];
      freePointer++;
    }
  }

  Node* newRootNewAddress = newRoot->newAddress;
  //std::cout << " | " << newRootNewAddress << " ";

  for(Node& node : tree.tree){
    if(node.mark == marked){
      if(node.parent){
        assert(&node == newRoot || node.parent->mark == marked);
        node.parent = node.parent->newAddress;
      }
      for(int i=0; i<node.children.size(); i++){
        if(node.children[i].child){
          assert(node.children[i].child->mark == marked);
          node.children[i].child = node.children[i].child->newAddress;
        }
      }
    }
  }

  for(uint32_t i=0; i<tree.tree.size(); i++){
    Node* livePointer = &tree.tree[i];
    if(livePointer->mark == marked){
      *(livePointer->newAddress) = *livePointer;
    }
    livePointer++;
  }

  tree.tree.resize(markedNodes);

  return newRootNewAddress;
}

Edge* selectEdge(Node* parent, bool isRoot){
  float maxPriority = -2;
  uint8_t maxPriorityNodeIndex = 0;

  const float parentVisitsTerm = (isRoot ? Aurora::options["rootExplorationFactor"].value : Aurora::options["explorationFactor"].value)*std::sqrt(std::log(parent->visits));

  //const float parentVisitsTerm = eP[5]*powl(eP[2]*logl(eP[0]*parent->visits+eP[1])+eP[3], eP[4])+eP[6];

  // while(currNode != nullptr){
  //   if(true){
  //     currNode->sPriority = -currNode->value+parentVisitsTerm/(eP[11]*powl(eP[7]*currNode->visits+eP[8], eP[9])+eP[10]);
  for(int i=0; i<parent->children.size(); i++){
    Node* currNode = parent->children[i].child;
    Edge currEdge = parent->children[i];

    float currPriority = -currEdge.value+parentVisitsTerm/std::sqrt(currNode ? currNode->visits : 1);

    assert(currPriority>=-1);

    if(currPriority>maxPriority){
      maxPriority = currPriority;
      maxPriorityNodeIndex = i;
    }
  }

  return &parent->children[maxPriorityNodeIndex];
}

void expand(Tree& tree, Node* parent, chess::MoveList& moves){
  if(moves.size()==0){return;}

  // tree.tree.push_back(Node(parent, 0, moves[0], parent->depth+1));
  // parent->firstChild = &tree.tree[tree.tree.size()-1];
  // parent->firstChild->mark = parent->mark;
  // Node* currNode = parent->firstChild;

  parent->children.resize(moves.size());

  for(uint16_t i=0; i<moves.size(); i++){
    parent->children[i] = Edge(moves[i]);
  }

  #if DATAGEN == 0
  if(parent->depth+1>seldepth){seldepth = parent->depth+1;}
  #endif
}

template<int numHiddenNeurons>
float playout(chess::Board& board, evaluation::NNUE<numHiddenNeurons>& nnue){
  chess::gameStatus _gameStatus = chess::getGameStatus(board, chess::isLegalMoves(board));
  assert(-1<=_gameStatus && 2>=_gameStatus);
  if(_gameStatus != chess::ONGOING){
    return _gameStatus;
  }

  //std::cout << evaluation::evaluate(board, nnue) << " ";

  float eval = std::max(std::min(std::atan(evaluation::evaluate(board, nnue)*Aurora::options["evalScaleFactor"].value/100.0)/1.57079633, 1.0),-1.0)*0.999999;
  assert(-1<=eval && 1>=eval);
  return eval;
}

Edge findBestEdge(Node* parent){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
  Edge currBestMove = parent->children[0];

  for(int i=0; i<parent->children.size(); i++){
    if(parent->children[i].value < currBestValue){
      currBestValue = parent->children[i].value;
      currBestMove = parent->children[i];
    }
  }

  return currBestMove;
}

Node* findBestChild(Node* parent){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
  Node* currBestMove = parent->children[0].child;

  for(int i=0; i<parent->children.size(); i++){
    if(parent->children[i].value < currBestValue){
      currBestValue = parent->children[i].value;
      currBestMove = parent->children[i].child;
    }
  }

  return currBestMove;
}

float findBestValue(Node* parent){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective

  for(int i=0; i<parent->children.size(); i++){
    currBestValue = std::min(currBestValue, parent->children[i].value);
  }

  return currBestValue;
}

int previousVisits = 0;
int previousElapsed = 0;

void printSearchInfo(Node* root, std::chrono::steady_clock::time_point start, bool finalResult){
  if(Aurora::options["outputLevel"].value==3){
    std::cout << "NODES: " << root->visits;
    #if DATAGEN == 0
    std::cout << " SELDEPTH: " << seldepth-root->depth <<"\n";
    #endif
    for(int i=0; i<root->children.size(); i++){
      Edge currEdge = root->children[i];
      std::cout << currEdge.edge.toStringRep() << ": Q:" << -currEdge.value << " N:" << (currEdge.child ? currEdge.child->visits : 1) <<  " PV:";
      Node* pvNode = root->children[i].child;
      while(pvNode && pvNode->children.size() > 0){
        Edge pvEdge = findBestEdge(pvNode);
        std::cout << pvEdge.edge.toStringRep() << " ";
        pvNode = pvEdge.child;
      }
      std::cout << std::endl;
    }
  }

  if(Aurora::options["outputLevel"].value >= 2 || (finalResult && Aurora::options["outputLevel"].value >= 1)){
    std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;

    std::cout << "info ";
      #if DATAGEN == 0
      std::cout << "depth " << seldepth-root->depth << " ";
      #endif
      std::cout << "nodes " << root->visits <<
      " score cp " << fminf(fmaxf(round(tan(-fminf(fmaxf(findBestValue(root), -0.9999), 0.9999)*1.57079633)*100), -100000), 100000) <<
      " nps " << round((root->visits-previousVisits)/(elapsed.count()-previousElapsed)) <<
      " time " << round(elapsed.count()*1000) <<
      " pv ";
    Node* pvNode = root;
    while(pvNode && pvNode->children.size() > 0){
      Edge pvEdge = findBestEdge(pvNode);
      std::cout << pvEdge.edge.toStringRep() << " ";
      pvNode = pvEdge.child;
    }
    std::cout << std::endl;

    previousVisits = root->visits; previousElapsed = elapsed.count();
  }
}

void backpropagate(float result, std::vector<Edge*>& edges, uint8_t visits, bool runFindBestMove, bool continueBackprop, bool forceResult){
  //Backpropagate results
  if(edges.size() == 0){return;}

  Edge* currEdge = edges.back();
  edges.pop_back();

  currEdge->child->visits+=visits;
  currEdge->child->updatePriority = true;

  float oldCurrNodeValue = 2;

  if(backpropStrat == AVERAGE){
    assert(0);
    // currEdge->value = (currEdge->value*currEdge->visits+result)/(currEdge->visits+1);
    // result = -result;
  }
  else if(backpropStrat == MINIMAX){
    //We only need to backpropagate two types of results here: the current best child becomes worse, or there is a new best child
    if(continueBackprop){
      //If currEdge is the best move and is backpropagated to become worse, we need to run findBestValue for the parent of currEdge
      oldCurrNodeValue = 2;
      if(currEdge->child->parent && edges.size() > 0 && -currEdge->value == edges.back()->value){oldCurrNodeValue = currEdge->value;}

      //If the result is worse than the current value, there is no point in continuing the backpropagation, other than to add visits to the nodes
      if(result <= currEdge->value && !runFindBestMove && !forceResult){
        continueBackprop = false;
        backpropagate(result, edges, visits, runFindBestMove, continueBackprop, false);
        return;
      }

      currEdge->value = runFindBestMove ? -findBestValue(currEdge->child) : result;

      assert(-1<=currEdge->value && 1>=currEdge->value);

      runFindBestMove = currEdge->value > oldCurrNodeValue; //currEdge(which used to be the best child)'s value got worse from currEdge's parent's perspective

      result = -currEdge->value;
    }
  }

  backpropagate(result, edges, visits, runFindBestMove, continueBackprop, false);
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

  evaluation::NNUE<NNUEhiddenNeurons> nnue(evaluation::_NNUEparameters);

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
    currNode = root; currNode->visits++;
    chess::Board board = rootBoard;
    Edge* currEdge;
    std::vector<Edge*> traversePath;
    //Traverse the search tree
    while(currNode->children.size() > 0){
      currEdge = selectEdge(currNode, currNode == root);
      traversePath.push_back(currEdge);
      chess::makeMove(board, currEdge->edge);
      if(currEdge->child == nullptr){
        tree.tree.push_back(Node(currNode, 0, currNode->depth+1));
        currEdge->child = &tree.tree[tree.tree.size()-1];
        currEdge->child->mark = currNode->mark;
        currEdge->child->visits = 1;
      }
      currNode = currEdge->child;
    }
    //Expand & Backpropagate new values
    if(currNode->isTerminal){
      backpropagate(currEdge->value, traversePath, 1, false, true, true);
    }
    else{
      //Reached a leaf node
      chess::MoveList moves(board);
      if(chess::getGameStatus(board, moves.size()!=0) != chess::ONGOING){assert(currEdge->value>=-1); currNode->isTerminal=true; continue;}
      expand(tree, currNode, moves); //Create new child nodes
      //Simulate for all new nodes
      Node* parentNode = currNode; //This will be the root of the backpropagation
      float currBestValue = 2; //Find and only backpropagate the best value we end up finding
      nnue.refreshAccumulator(board);
      std::array<std::array<int16_t, NNUEhiddenNeurons>, 2> currAccumulator = nnue.accumulator;
      for(int i=0; i<parentNode->children.size(); i++){
        currEdge = &parentNode->children[i];
        chess::Board movedBoard = board;
        nnue.accumulator = currAccumulator;

        nnue.updateAccumulator(movedBoard, currEdge->edge);
        float result = playout(movedBoard, nnue);
        assert(-1<=result && 1>=result);
        currEdge->value = result;

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
    std::cout << "\nbestmove " << findBestEdge(root).edge.toStringRep() << std::endl;
  #endif

  return root;
}
//Same as chess::makeMove except we move the root so we can keep nodes from an earlier search
//Parameter "board" must be different than parameter "rootBoard"
void makeMove(chess::Board& board, chess::Move move, chess::Board& rootBoard, Node*& root, Tree& tree){
  if(root == nullptr || zobrist::getHash(board) != zobrist::getHash(rootBoard)){chess::makeMove(board, move); return;}

  chess::makeMove(board, move);

  Edge newRootEdge = Edge(chess::Move());
  for(int i=0; i<root->children.size(); i++){
    newRootEdge = root->children[i];
    if(newRootEdge.edge == move){break;}
  }
  Node* newRoot = newRootEdge.child;

  if(newRoot == nullptr){root = nullptr; destroyTree(tree); return;}

  root = moveRootToChild(tree, newRoot, root);

  root->parent = nullptr; root->index = 0; root->visits--;//Visits needs to be subtracted by 1 to remove the visit which added the node

  chess::makeMove(rootBoard, move);
}

}//namespace search