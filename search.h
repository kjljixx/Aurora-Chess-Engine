#pragma once
#include "evaluation.h"
#include <fstream>
#include <time.h>
#include <math.h>
#include <memory>
#include <chrono>

//Set to 1 if you want to build a version of Aurora which generates data, 2 for generating data while playing (cutechess), 0 for the normal version.
#define DATAGEN 0
#if DATAGEN >= 1
  #include <windows.h>
  LPCSTR dataFilePath = "C:/Users/kjlji/OneDrive/Documents/VSCode/C++/AuroraChessEngine-main/data/version0.10.1-50000npm.auroradata";
#endif

namespace search{

//parameters for search
enum backpropagationStrategy{AVERAGE, MINIMAX};
backpropagationStrategy backpropStrat = MINIMAX;
float outputLevel = 2; //outputLevel:
                       //0: only output bestmove at end of search
                       //1: ouput bestmove and info at end of search
                       //2: output bestmove and info at end of search and output info every 2 seconds
                       //3: output bestmove and info at end of search and output info + verbose move stats every 2 seconds

//Tuned with Weather Factory with 28256 iterations(games) at 5+0.05
float explorationFactor = 0.18672279654288734;
float evalScaleFactor = 1;

uint8_t seldepth = 0;

void init(){
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
  chess::Move edge;
  uint32_t visits;
  uint32_t displayVisits;
  float value;
  bool isTerminal;
  uint8_t depth;
  float sPriority;
  bool updatePriority;

  Node(Node* parent, uint8_t index, chess::Move edge, uint8_t depth) :
  parent(parent), index(index),
  firstChild(nullptr), nextSibling(nullptr),
  visits(0), displayVisits(0), value(-2), edge(edge), isTerminal(false), depth(depth), sPriority(-1), updatePriority(true) {}

  Node() : parent(nullptr), index(0), firstChild(nullptr), nextSibling(nullptr), visits(0), displayVisits(0), value(-2), edge(chess::Move()), isTerminal(false), depth(0), sPriority(-1), updatePriority(true) {}

  Node* getChildByIndex(uint8_t index){
    Node* currNode = firstChild;
    
    for(int i=0; i<index; i++){
      currNode = currNode->nextSibling;
    }

    return currNode;    
  }
};

Node* root = nullptr;
chess::Board searchRootBoard;

void destroyTree(Node* node){
  if(node){
    destroyTree(node->firstChild);
    destroyTree(node->nextSibling);

    delete node;
  }
}
//destroytree except we don't delete the siblings of the node
void destroySubtree(Node* node){
  if(node){
    destroyTree(node->firstChild);

    delete node;
  }
}
//the node argument passed in moveRootToChild should be the first child of the root
void moveRootToChild(Node* node, Node* newRoot){
  if(node){
    moveRootToChild(node->nextSibling, newRoot);
    
    if(node!=newRoot && node!=root){destroySubtree(node);}
  }
}

Node* selectChild(Node* parent){
  Node* currNode = parent->firstChild;
  float maxPriority = -2;
  uint8_t maxPriorityNodeIndex = 0;

  const float parentVisitsTerm = sqrtl(explorationFactor*logl(parent->visits));

  while(currNode != nullptr){
    if(true){
      currNode->sPriority = -currNode->value+parentVisitsTerm/sqrtl(currNode->visits);
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

void expand(Node* parent, chess::MoveList& moves){
  if(moves.size()==0){return;}

  parent->firstChild = new Node(parent, 0, moves[0], parent->depth+1);
  Node* currNode = parent->firstChild;

  for(int i=1; i<moves.size(); i++){
    currNode->nextSibling = new Node(parent, i, moves[i], parent->depth+1);
    currNode = currNode->nextSibling;
  }

  if(parent->depth+1>seldepth){seldepth = parent->depth+1;}
}

float playout(chess::Board& board, Node* currNode){
  chess::gameStatus _gameStatus = chess::getGameStatus(board, chess::isLegalMoves(board));
  assert(-1<=_gameStatus && 2>=_gameStatus);
  if(_gameStatus != chess::ONGOING){
    if(_gameStatus == chess::LOSS){return _gameStatus+0.00000001*currNode->depth;}
    return _gameStatus;
  }

  float eval = fmaxf(fminf(atan(evaluation::evaluate(board)*evalScaleFactor/100.0)/1.56375, 1),-1)*0.999999;
  assert(-1<=eval && 1>=eval);
  return eval;
}

Node* findBestChild(Node* parent){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
  Node* currBestMove;

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
    if(currNodeValue>=-1){currBestValue = fminf(currBestValue, currNodeValue);}

    currNode = currNode->nextSibling;
  }

  return currBestValue;
}

int previousVisits = 0;
int previousElapsed = 0;

void printSearchInfo(Node* root, std::chrono::_V2::steady_clock::time_point start, bool finalResult){
  Node* currNode = root;

  if(outputLevel==3){
    std::cout << "\nNODES: " << root->displayVisits << " SELDEPTH: " << seldepth-root->depth <<"\n";
    currNode = currNode->firstChild;
    while(currNode != nullptr){
    std::cout << currNode->edge.toStringRep() << ": Q:" << -currNode->value << " N:" << currNode->displayVisits << " SP:" << currNode->sPriority <<  " PV:";
    Node* pvNode = currNode;
    while(pvNode->firstChild != nullptr){
      pvNode = findBestChild(pvNode);
      std::cout << pvNode->edge.toStringRep() << " ";
    }
    std::cout << "\n";
    currNode = currNode->nextSibling;
    }
  }

  if(outputLevel >= 2 || finalResult && outputLevel >= 1){
    std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;

    std::cout << "\ninfo depth " << seldepth-root->depth <<
      " nodes " << root->displayVisits <<
      " score cp " << round(tan(-findBestValue(root)*1.56375)*100) <<
      " nps " << round((root->displayVisits-previousVisits)/(elapsed.count()-previousElapsed)) <<
      " time " << round(elapsed.count()*1000) <<
      " pv ";
    currNode = root;
    while(currNode->firstChild != nullptr){
      currNode = findBestChild(currNode);
      std::cout << currNode->edge.toStringRep() << " ";
    }

    previousVisits = root->displayVisits; previousElapsed = elapsed.count();
  }
}

void backpropagate(float result, Node* currNode, uint8_t visits){
  //Backpropogate results

  bool runFindBestMove = false;
  bool continueBackprop = true;
  float oldCurrNodeValue = 2;

  if(backpropStrat == MINIMAX){
    if(currNode->parent && -currNode->value == currNode->parent->value){oldCurrNodeValue = currNode->value;}

    currNode->value = result;
    assert(-1<=currNode->value && 1>=currNode->value);

    runFindBestMove = currNode->value > oldCurrNodeValue;
    result = -result;
    currNode->visits+=1;
    currNode->displayVisits+=visits;
    currNode->updatePriority = true;
    currNode = currNode->parent;
  }

  while(currNode != nullptr){
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
        if(result <= currNode->value && !runFindBestMove){continueBackprop = false; continue;}

        currNode->value = runFindBestMove ? -findBestValue(currNode) : result;

        assert(-1<=currNode->value && 1>=currNode->value);

        runFindBestMove = currNode->value > oldCurrNodeValue; //currNode(which used to be the best child)'s value got worse from currNode's parent's perspective

        result = -currNode->value;
      }
    }

    currNode->visits+=1;
    currNode->displayVisits+=visits;
    currNode->updatePriority = true;
    currNode = currNode->parent;
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

void search(const chess::Board& rootBoard, timeManagement tm){
  auto start = std::chrono::steady_clock::now();

  if(!root){root = new Node();}

  seldepth = 0;

  searchRootBoard = rootBoard;

  chess::Colors ourSide = rootBoard.sideToMove;

  Node* currNode = root;
  
  int lastNodeCheck = 1;
  std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;
  previousVisits = root->visits;
  previousElapsed = 0;

  while((tm.tmType == FOREVER) || (elapsed.count()<tm.limit && tm.tmType == TIME) || (root->displayVisits<tm.limit && tm.tmType == NODES)){
    currNode = root;
    chess::Board board = rootBoard;
    //Traverse the search tree
    while(currNode->firstChild != nullptr){
      currNode = selectChild(currNode);
      chess::makeMove(board, currNode->edge);
    }
    //Expand & Backpropagate new values
    if(currNode->isTerminal){
      backpropagate(currNode->value, currNode, 1);
    }
    else{
      //Reached a leaf node
      chess::MoveList moves(board);
      if(chess::getGameStatus(board, moves.size()!=0) != chess::ONGOING){assert(currNode->value>=-1); currNode->isTerminal=true; continue;}
      expand(currNode, moves);
      //Simulate for all new nodes
      Node* parentNode = currNode; //This will be the root of the backpropagation
      currNode = currNode->firstChild;
      float currBestValue = 2; //Find and only backpropagate the best value
      while(currNode != nullptr){
        chess::Board movedBoard = board;

        chess::makeMove(movedBoard, currNode->edge);
        float result = playout(movedBoard, currNode);
        assert(-1<=result && 1>=result);
        currNode->value = result;
        currNode->visits = 1;
        currNode->displayVisits = 1;
        currNode->updatePriority = true;

        currBestValue = fminf(currBestValue, result);

        currNode = currNode->nextSibling;
      }
      //Backpropagate best value
      backpropagate(-currBestValue, parentNode, moves.size());
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

  #if DATAGEN >= 1
    //Data Generation code
    HANDLE dataFile = CreateFileA(dataFilePath, FILE_APPEND_DATA, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    const std::string data = searchRootBoard.getFen() + " " + std::to_string(fminf(((searchRootBoard.sideToMove ? -root->value : root->value)+1)/2, 0.999999)) + "\n";
    DWORD dwBytesToWrite = (DWORD)strlen(data.c_str());
    DWORD dwBytesWritten = 0;
    WriteFile(dataFile, data.c_str(), dwBytesToWrite, &dwBytesWritten, NULL);
    CloseHandle(dataFile);
  #endif
}
//Same as chess::makeMove except we move the root so we can keep nodes from an earlier search
void makeMove(chess::Board& board, chess::Move move){
  if(root == nullptr || zobrist::getHash(board) != zobrist::getHash(searchRootBoard)){chess::makeMove(board, move); return;}

  chess::makeMove(board, move);

  Node* newRoot = root->firstChild;
  while(newRoot != nullptr){
    if(newRoot->edge == move){break;}
    newRoot = newRoot->nextSibling;
  }

  if(newRoot == nullptr){root = nullptr; return;}

  moveRootToChild(root->firstChild, newRoot);
  newRoot->parent = nullptr; newRoot->nextSibling = nullptr; newRoot->index = 0; newRoot->edge = chess::Move(); newRoot->visits--;//Visits needs to be subtracted by 1 to remove the visit which added the node

  chess::makeMove(searchRootBoard, move);

  root = newRoot;
}

}//namespace search