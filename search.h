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

void init(){
  lookupTables::init();
  srand(time(NULL));
}

struct Node{
  Node* parent;
  uint8_t index;
  std::unique_ptr<Node> firstChild;
  std::unique_ptr<Node> nextSibling;
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
  Node* currNode = parent->firstChild.get();
  float maxPriority = -1;
  Node* maxPriorityNode = currNode;

  const float parentVisitsTerm = sqrtl(logl(parent->visits));

  while(currNode != nullptr){
    float currPriority = currNode->value+(2*parentVisitsTerm)/sqrtl(currNode->visits);

    if(currPriority>maxPriority){
      maxPriority = currPriority;
      maxPriorityNode = currNode;
    }

    currNode = currNode->nextSibling.get();
  }

  return maxPriorityNode;
}

void expand(Node* parent, chess::MoveList& moves){
  if(moves.size()==0){return;}

  parent->firstChild = std::unique_ptr<Node>(new Node(parent, 0, moves[0], parent->depth+1));
  Node* currNode = parent->firstChild.get();
  nodes++;

  for(int i=1; i<moves.size(); i++){
    currNode->nextSibling = std::unique_ptr<Node>(new Node(parent, i, moves[i], parent->depth+1));
    currNode = currNode->nextSibling.get();
    nodes++;
  }

  if(parent->depth+1>seldepth){seldepth = parent->depth+1;}
}

float playout(chess::Board& board){
  chess::gameStatus _gameStatus = chess::getGameStatus(board, chess::isLegalMoves(board));
  if(_gameStatus != chess::ONGOING){return _gameStatus;}
  
  return evaluation::evaluate(board) + (rand() % 2000)/2000000.0; //add a small random offset so each evaluation is slightly different
}

Node* findBestMove(Node* parent){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
  Node* currBestMove;

  Node* currNode = parent->firstChild.get();
  while(currNode != nullptr){
    if(currNode->value < currBestValue){
      currBestValue = currNode->value;
      currBestMove = currNode;
    }

    currNode = currNode->nextSibling.get();
  }

  return currBestMove;
}

void backpropagate(float result, Node* currNode){
  //Backpropogate results
  //result = -result; Negate the reverse since it is the evaluation for the opponent of the currnode

  while(currNode != nullptr){
    if(backpropStrat == AVERAGE){
      currNode->value = (currNode->value*currNode->visits+result)/(currNode->visits+1);
      result = -result;
    }
    else if(backpropStrat == MINIMAX){
      if(currNode->firstChild == nullptr){currNode->value = result;}//This is for the case where currNode is a leaf node
      else{
        currNode->value = -findBestMove(currNode)->value;
      }
    }

    currNode->visits++;
    currNode = currNode->parent;
  }
}

void search(const chess::Board& rootBoard, uint32_t maxNodes){
  auto start = std::chrono::steady_clock::now();

  nodes = 0;

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
      currNode = currNode->firstChild.get();
      while(currNode != nullptr){
        chess::Board movedBoard = board;

        movedBoard.makeMove(currNode->edge);
        float result = playout(movedBoard);
        backpropagate(result, currNode);

        currNode = currNode->nextSibling.get();
      }
    }

    //output some information on the search occasionally
    currNode = &root;
    if(nodes >= lastNodeCheck*800000){
      lastNodeCheck++;
      
       std::cout << "\nNODES: " << nodes << " SELDEPTH: " << int(seldepth) <<"\n";
       currNode = root.firstChild.get();
       while(currNode != nullptr){
         std::cout << currNode->edge.toStringRep() << ": Q:" << currNode->value << " N:" << currNode->visits << "\n";
         currNode = currNode->nextSibling.get();
       }

      currNode = &root;
      std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;
  
      std::cout << "\ninfo nodes " << nodes <<
        " nps " << round(nodes/elapsed.count()) <<
        " time " << round(elapsed.count()*1000) <<
        " score cp " << round((log(2/(findBestMove(&root)->value+1)-1)/-1.946)*100) <<  " wdl " << round(((findBestMove(&root)->value+1)/2)*1000) << " 0 " << 1000-round(((findBestMove(&root)->value+1)/2)*1000) << 
        " pv ";
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
  std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;

  std::cout << "\ninfo nodes " << nodes <<
    " nps " << round(nodes/elapsed.count()) <<
    " time " << round(elapsed.count()*1000) <<
    " score cp " << round((log(2/(findBestMove(&root)->value+1)-1)/-1.946)*100) <<  " wdl " << round(((findBestMove(&root)->value+1)/2)*1000) << " 0 " << 1000-round(((findBestMove(&root)->value+1)/2)*1000) << 
    " pv ";
  while(currNode->firstChild != nullptr){
    currNode = findBestMove(currNode);
    std::cout << currNode->edge.toStringRep() << " ";
  }
  std::cout << "\nbestmove " << findBestMove(&root)->edge.toStringRep() << " ponder " << findBestMove(findBestMove(&root))->edge.toStringRep() << "\n";

}

}//namespace search