#include "evaluation.h"
#include <time.h>
#include <math.h>
#include <memory>
#include <chrono>

namespace search{

enum backpropagationStrategy{AVERAGE, MINIMAX};
backpropagationStrategy backpropStrat = MINIMAX;

uint32_t nodes = 0;
uint8_t seldepth = 0;
chess::Colors ourSide;

void init(){
  lookupTables::init();
  srand(time(NULL));
}

struct Node{
  Node* parent;
  uint8_t index;
  Node* firstChild;
  Node* nextSibling;
  chess::Move edge;
  uint32_t visits;
  float value;
  bool isTerminal;
  uint8_t depth;

  Node(Node* parent, uint8_t index, chess::Move edge, uint8_t depth) :
  parent(parent), index(index),
  firstChild(nullptr), nextSibling(nullptr),
  visits(0), value(-2), edge(edge), isTerminal(false), depth(depth) {}

  Node() : parent(nullptr), index(0), firstChild(nullptr), nextSibling(nullptr), visits(0), value(-2), edge(chess::Move()), isTerminal(false), depth(0) {}
};

Node* selectChild(Node* parent){
  Node* currNode = parent->firstChild;
  float maxPriority = -1;
  Node* maxPriorityNode = currNode;

  const float parentVisitsTerm = sqrtl(logl(parent->visits));

  while(currNode != nullptr){
    //Since our value ranges from -1 to 1 instead of 0 to 1, the c value in the ucb1 formula becomes 4
    float currPriority = -currNode->value+(4*parentVisitsTerm)/sqrtl(currNode->visits);

    if(currPriority>maxPriority){
      maxPriority = currPriority;
      maxPriorityNode = currNode;
    }

    currNode = currNode->nextSibling;
  }

  return maxPriorityNode;
}

void expand(Node* parent, chess::MoveList& moves){
  if(moves.size()==0){return;}

  parent->firstChild = new Node(parent, 0, moves[0], parent->depth+1);
  Node* currNode = parent->firstChild;
  nodes++;

  for(int i=1; i<moves.size(); i++){
    currNode->nextSibling = new Node(parent, i, moves[i], parent->depth+1);
    currNode = currNode->nextSibling;
    nodes++;
  }

  if(parent->depth+1>seldepth){seldepth = parent->depth+1;}
}

float playout(chess::Board& board){
  chess::gameStatus _gameStatus = chess::getGameStatus(board, chess::isLegalMoves(board));
  if(_gameStatus != chess::ONGOING){return _gameStatus;}
  
  return fmin(fmax(evaluation::evaluate(board)+(rand() % 2048)/10000000.0, -0.999), 0.999);
}

Node* findBestMove(Node* parent){
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
    if(currNodeValue < currBestValue){
      currBestValue = currNodeValue;
    }

    currNode = currNode->nextSibling;
  }

  return currBestValue;
}

void backpropagate(float result, Node* currNode){
  //Backpropogate results

  bool runFindBestMove = false;
  float oldCurrNodeValue;

  while(currNode != nullptr){
    if(backpropStrat == AVERAGE){
      currNode->value = (currNode->value*currNode->visits+result)/(currNode->visits+1);
      result = -result;
    }
    else if(backpropStrat == MINIMAX){
      //If currNode is the best move and is backpropagated to become worse, we need to run findBestMove for the parent of currNode
      oldCurrNodeValue = -2;
      if(currNode->parent && currNode->value == currNode->parent->value){oldCurrNodeValue = currNode->value;}

      if(currNode->firstChild == nullptr){currNode->value = result;}//This is for the case where currNode is a leaf node
      else{
        //If the result is less than the current value, there is no point in continuing the backpropagation
        if(-result >= currNode->value && !runFindBestMove){break;}

        currNode->value = runFindBestMove ? -findBestValue(currNode) : fmax(-result, currNode->value);
      }

      runFindBestMove = currNode->value > oldCurrNodeValue;

      result = currNode->value;
    }

    currNode->visits++;
    currNode = currNode->parent;
  }
}
//The main search function
void search(const chess::Board& rootBoard, uint32_t maxNodes){
  auto start = std::chrono::steady_clock::now();

  nodes = 0;
  seldepth = 0;
  ourSide = rootBoard.sideToMove;

  Node root = Node(nullptr, 0, chess::Move(), 0); root.visits = 1;
  Node* currNode = &root; 
  
  int lastNodeCheck = 0;

  while(nodes<maxNodes){
    currNode = &root;
    chess::Board board = rootBoard;
    while(currNode->firstChild != nullptr){
      currNode = selectChild(currNode);
      board.makeMove(currNode->edge);
    }
    if(currNode->isTerminal){
      backpropagate(currNode->value, currNode);
      nodes++;
    }
    else{
      //Reached a leaf node
      chess::MoveList moves(board);
      if(chess::getGameStatus(board, moves.size()!=0) != chess::ONGOING){currNode->isTerminal=true; continue;}
      expand(currNode, moves);
      //Simulate for all new nodes
      currNode = currNode->firstChild;
      while(currNode != nullptr){
        chess::Board movedBoard = board;

        movedBoard.makeMove(currNode->edge);
        float result = playout(movedBoard);
        backpropagate(result, currNode);

        currNode = currNode->nextSibling;
      }
    }

    //output some information on the search occasionally
    if(nodes >= lastNodeCheck*500000){
      currNode = &root;
      lastNodeCheck++;
      
       std::cout << "\nNODES: " << nodes << " SELDEPTH: " << int(seldepth) <<"\n";
       currNode = root.firstChild;
       while(currNode != nullptr){
        std::cout << currNode->edge.toStringRep() << ": Q:" << -currNode->value << " N:" << currNode->visits << " PV:";
        Node* pvNode = currNode;
        while(pvNode->firstChild != nullptr){
          pvNode = findBestMove(pvNode);
          std::cout << pvNode->edge.toStringRep() << " ";
        }
        std::cout << "\n";
        currNode = currNode->nextSibling;
       }

      std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;
  
      std::cout << "\ninfo nodes " << nodes <<
        " nps " << round(nodes/elapsed.count()) <<
        " time " << round(elapsed.count()*1000) <<
        " score cp " << round((log(2/(-findBestValue(&root)+1)-1)/-1.946)*100) <<  " wdl " << round(((-findBestValue(&root)+1)/2)*1000) << " 0 " << 1000-round(((-findBestValue(&root)+1)/2)*1000) << 
        " pv ";
      currNode = &root;
      while(currNode->firstChild != nullptr){
        currNode = findBestMove(currNode);
        std::cout << currNode->edge.toStringRep() << " ";
      }
    }
  }
  //Output the final result of the search
  // std::cout << "\nNODES: " << nodes << "\n";
  // currNode = root.firstChild.get();
  // while(currNode != nullptr){
  //   std::cout << currNode->edge.toStringRep() << ": Q:" << currNode->value << " N:" << currNode->visits << "\n";
  //   currNode = currNode->nextSibling.get();
  // }
  currNode = &root;
  lastNodeCheck++;
  
    std::cout << "\nNODES: " << nodes << " SELDEPTH: " << int(seldepth) <<"\n";
    currNode = root.firstChild;
    while(currNode != nullptr){
      std::cout << currNode->edge.toStringRep() << ": Q:" << -currNode->value << " N:" << currNode->visits << " PV:";
      Node* pvNode = currNode;
      while(pvNode->firstChild != nullptr){
        pvNode = findBestMove(pvNode);
        std::cout << pvNode->edge.toStringRep() << " ";
      }
      std::cout << "\n";
      currNode = currNode->nextSibling;
    }

  std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;

  std::cout << "\ninfo nodes " << nodes <<
    " nps " << round(nodes/elapsed.count()) <<
    " time " << round(elapsed.count()*1000) <<
    " score cp " << round((log(2/(-findBestValue(&root)+1)-1)/-1.946)*100) <<  " wdl " << round(((-findBestValue(&root)+1)/2)*1000) << " 0 " << 1000-round(((-findBestValue(&root)+1)/2)*1000) << 
    " pv ";
  currNode = &root;
  while(currNode->firstChild != nullptr){
    currNode = findBestMove(currNode);
    std::cout << currNode->edge.toStringRep() << " ";
  }
  std::cout << "\nbestmove " << findBestMove(&root)->edge.toStringRep() << " ponder " << findBestMove(findBestMove(&root))->edge.toStringRep() << "\n";
}

}//namespace search