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

struct Node{
  Node* parent;
  uint8_t index;
  Node* firstChild;
  Node* nextSibling;
  uint32_t visits;
  float value;
  chess::Move edge;
  bool isTerminal;
  uint8_t depth;
  float sPriority;
  bool updatePriority;

  //For Tree Reuse
  Node* newAddress = nullptr;
  bool mark = false;

  Node(Node* parent, uint8_t index, chess::Move edge, uint8_t depth) :
  parent(parent), index(index),
  firstChild(nullptr), nextSibling(nullptr),
  visits(0), value(-2), edge(edge), isTerminal(false), depth(depth), sPriority(-1), updatePriority(true) {}

  Node() : parent(nullptr), index(0), firstChild(nullptr), nextSibling(nullptr), visits(0), value(-2), edge(chess::Move()), isTerminal(false), depth(0), sPriority(-1), updatePriority(true) {}

  Node* getChildByIndex(uint8_t index){
    Node* currNode = firstChild;
    
    for(int i=0; i<index; i++){
      currNode = currNode->nextSibling;
    }

    return currNode;    
  }
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
    markedNodes += markSubtree(node->firstChild, false, unmarked);
    assert((!node->firstChild) || (node->firstChild->mark == !unmarked));
    if(!isSubtreeRoot){
      markedNodes += markSubtree(node->nextSibling, false, unmarked);
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
      if(node.firstChild){
        assert(node.firstChild->mark == marked);
        node.firstChild = node.firstChild->newAddress;
      }
      if(node.nextSibling){
        assert(&node == newRoot || node.nextSibling->mark == marked);
        node.nextSibling = node.nextSibling->newAddress;
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

Node* selectChild(Node* parent, bool isRoot){
  Node* currNode = parent->firstChild;
  float maxPriority = -2;
  uint8_t maxPriorityNodeIndex = 0;

  const float parentVisitsTerm = (isRoot ? Aurora::options["rootExplorationFactor"].value : Aurora::options["explorationFactor"].value)*std::sqrt(std::log(parent->visits));

  //const float parentVisitsTerm = eP[5]*powl(eP[2]*logl(eP[0]*parent->visits+eP[1])+eP[3], eP[4])+eP[6];

  // while(currNode != nullptr){
  //   if(true){
  //     currNode->sPriority = -currNode->value+parentVisitsTerm/(eP[11]*powl(eP[7]*currNode->visits+eP[8], eP[9])+eP[10]);
  while(currNode != nullptr){
    if(true){
      currNode->sPriority = -currNode->value+parentVisitsTerm/std::sqrt(currNode->visits);
      currNode->updatePriority = false;
    }
    float currPriority = currNode->sPriority;

    assert(currPriority>=-1);

    if(currPriority>maxPriority){
      maxPriority = currPriority;
      maxPriorityNodeIndex = currNode->index;
    }
    
    currNode = currNode->nextSibling;
  }

  return parent->getChildByIndex(maxPriorityNodeIndex);
}

void expand(Tree& tree, Node* parent, chess::MoveList& moves){
  if(moves.size()==0){return;}

  tree.tree.push_back(Node(parent, 0, moves[0], parent->depth+1));
  parent->firstChild = &tree.tree[tree.tree.size()-1];
  parent->firstChild->mark = parent->mark;
  Node* currNode = parent->firstChild;

  for(uint16_t i=1; i<moves.size(); i++){
    tree.tree.push_back(Node(parent, i, moves[i], parent->depth+1));
    currNode->nextSibling = &tree.tree[tree.tree.size()-1];
    currNode->nextSibling->mark = parent->mark;
    currNode = currNode->nextSibling;
  }

  #if DATAGEN == 0
  if(parent->depth+1>seldepth){seldepth = parent->depth+1;}
  #endif
}

float playout(chess::Board& board, Node* currNode, evaluation::NNUE& nnue){
  chess::gameStatus _gameStatus = chess::getGameStatus(board, chess::isLegalMoves(board));
  assert(-1<=_gameStatus && 2>=_gameStatus);
  if(_gameStatus != chess::ONGOING){
    currNode->isTerminal = true;
    if(_gameStatus == chess::LOSS){return _gameStatus+0.00000001*currNode->depth;}
    return _gameStatus;
  }

  //std::cout << evaluation::evaluate(board, nnue) << " ";

  float eval = std::max(std::min(std::atan(evaluation::evaluate(board, nnue)*Aurora::options["evalScaleFactor"].value/100.0)/1.57079633, 1.0),-1.0)*0.999999;
  assert(-1<=eval && 1>=eval);
  return eval;
}

Node* findBestChild(Node* parent){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
  Node* currBestMove = parent->firstChild;

  Node* currNode = parent->firstChild;
  while(currNode != nullptr){
    if(currNode->value < currBestValue){
      currBestValue = currNode->value;
      currBestMove = currNode;
    }

    currNode = currNode->nextSibling;
  }

  return currBestMove;
}

float findBestValue(Node* parent){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective

  Node* currNode = parent->firstChild;
  while(currNode != nullptr){
    float currNodeValue = currNode->value;
    if(currNodeValue>=-1){currBestValue = std::min(currBestValue, currNodeValue);}

    currNode = currNode->nextSibling;
  }

  return currBestValue;
}

int previousVisits = 0;
int previousElapsed = 0;

void printSearchInfo(Node* root, std::chrono::steady_clock::time_point start, bool finalResult){
  Node* currNode = root;

  if(Aurora::options["outputLevel"].value==3){
    std::cout << "\nNODES: " << root->visits;
    #if DATAGEN == 0
    std::cout << " SELDEPTH: " << seldepth-root->depth <<"\n";
    #endif
    currNode = currNode->firstChild;
    while(currNode != nullptr){
    std::cout << currNode->edge.toStringRep() << ": Q:" << -currNode->value << " N:" << currNode->visits << " SP:" << currNode->sPriority <<  " PV:";
    Node* pvNode = currNode;
    while(pvNode->firstChild != nullptr){
      pvNode = findBestChild(pvNode);
      std::cout << pvNode->edge.toStringRep() << " ";
    }
    std::cout << "\n";
    currNode = currNode->nextSibling;
    }
  }

  if(Aurora::options["outputLevel"].value >= 2 || (finalResult && Aurora::options["outputLevel"].value >= 1)){
    std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;

    std::cout << "\ninfo ";
      #if DATAGEN == 0
      std::cout << "depth " << seldepth-root->depth;
      #endif
      std::cout << " nodes " << root->visits <<
      " score cp " << round(tan(-findBestValue(root)*1.57079633)*100) <<
      " nps " << round((root->visits-previousVisits)/(elapsed.count()-previousElapsed)) <<
      " time " << round(elapsed.count()*1000) <<
      " pv ";
    currNode = root;
    while(currNode->firstChild != nullptr){
      currNode = findBestChild(currNode);
      std::cout << currNode->edge.toStringRep() << " ";
    }

    previousVisits = root->visits; previousElapsed = elapsed.count();
  }
}

void backpropagate(float result, Node* currNode, uint8_t visits, bool runFindBestMove, bool continueBackprop, bool forceResult){
  //Backpropagate results

  if(currNode == nullptr){return;}

  float oldCurrNodeValue = 2;

  if(backpropStrat == AVERAGE){
    currNode->value = (currNode->value*currNode->visits+result)/(currNode->visits+1);
    result = -result;
  }
  else if(backpropStrat == MINIMAX){
    //We only need to backpropagate two types of results here: the current best child becomes worse, or there is a new best child
    if(continueBackprop){
      //If currNode is the best move and is backpropagated to become worse, we need to run findBestValue for the parent of currNode
      oldCurrNodeValue = 2;
      if(currNode->parent && -currNode->value == currNode->parent->value){oldCurrNodeValue = currNode->value;}

      //If the result is worse than the current value, there is no point in continuing the backpropagation, other than to add visits to the nodes
      if(result <= currNode->value && !runFindBestMove && !forceResult){
        continueBackprop = false;
        currNode->visits+=visits;
        currNode->updatePriority = true;
        currNode = currNode->parent;

        backpropagate(result, currNode, visits, runFindBestMove, continueBackprop, false);
        return;
      }

      currNode->value = runFindBestMove ? -findBestValue(currNode) : result;

      assert(-1<=currNode->value && 1>=currNode->value);

      runFindBestMove = currNode->value > oldCurrNodeValue; //currNode(which used to be the best child)'s value got worse from currNode's parent's perspective

      result = -currNode->value;
    }
  }

  currNode->visits+=visits;
  currNode->updatePriority = true;
  currNode = currNode->parent;

  backpropagate(result, currNode, visits, runFindBestMove, continueBackprop, false);
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

  if(chess::getGameStatus(rootBoard, chess::isLegalMoves(rootBoard)) != chess::ONGOING){
    #if DATAGEN != 1
      std::cout << "bestmove a1a1\n";
    #endif
    return root;
  }

  while((tm.tmType == FOREVER) || (elapsed.count()<tm.limit && tm.tmType == TIME) || (root->visits<tm.limit && tm.tmType == NODES)){
    currNode = root;
    chess::Board board = rootBoard;
    //Traverse the search tree
    while(currNode->firstChild != nullptr){
      currNode = selectChild(currNode, currNode == root);
      chess::makeMove(board, currNode->edge);
    }
    //Expand & Backpropagate new values
    if(currNode->isTerminal){
      backpropagate(currNode->value, currNode, 1, false, true, true);
    }
    else{
      //Reached a leaf node
      chess::MoveList moves(board);
      if(chess::getGameStatus(board, moves.size()!=0) != chess::ONGOING){assert(currNode->value>=-1); currNode->isTerminal=true; continue;}
      expand(tree, currNode, moves); //Create new child nodes
      //Simulate for all new nodes
      Node* parentNode = currNode; //This will be the root of the backpropagation
      currNode = currNode->firstChild;
      float currBestValue = 2; //Find and only backpropagate the best value we end up finding
      nnue.refreshAccumulator(board);
      std::array<std::array<int16_t, NNUEhiddenNeurons>, 2> currAccumulator = nnue.accumulator;
      while(currNode != nullptr){
        chess::Board movedBoard = board;
        nnue.accumulator = currAccumulator;

        nnue.updateAccumulator(movedBoard, currNode->edge);
        float result = playout(movedBoard, currNode, nnue);
        //std::cout << "\nRESULT: " << result;
        assert(-1<=result && 1>=result);
        currNode->value = result;
        currNode->visits = 1;
        currNode->updatePriority = true;

        currBestValue = fminf(currBestValue, result);

        currNode = currNode->nextSibling;
      }
      //Backpropagate best value
      backpropagate(-currBestValue, parentNode, 1, false, true, true);
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
    std::cout << "\nbestmove " << findBestChild(root)->edge.toStringRep() << "\n";
  #endif

  return root;
}
//Same as chess::makeMove except we move the root so we can keep nodes from an earlier search
//Parameter "board" must be different than parameter "rootBoard"
void makeMove(chess::Board& board, chess::Move move, chess::Board& rootBoard, Node*& root, Tree& tree){
  if(root == nullptr || zobrist::getHash(board) != zobrist::getHash(rootBoard)){chess::makeMove(board, move); return;}

  chess::makeMove(board, move);

  Node* newRoot = root->firstChild;
  while(newRoot != nullptr){
    if(newRoot->edge == move){break;}
    newRoot = newRoot->nextSibling;
  }

  if(newRoot == nullptr){root = nullptr; destroyTree(tree); return;}

  root = moveRootToChild(tree, newRoot, root);

  root->parent = nullptr; root->nextSibling = nullptr; root->index = 0; root->edge = chess::Move(); root->visits--;//Visits needs to be subtracted by 1 to remove the visit which added the node

  chess::makeMove(rootBoard, move);
}

}//namespace search