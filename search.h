#include "uci.h"
#include <time.h>

namespace search{

void init(){
  lookupTables::init();
  srand(time(NULL));
}
//playout
chess::gameStatus playout(chess::Board board){
  chess::Colors ourColor = board.sideToMove;
  while(true){
    chess::MoveList moveList(board);

    chess::gameStatus _gameStatus = chess::getGameStatus(board, moveList);
    //std::cout << "hic";
    if(_gameStatus != chess::ONGOING){return chess::gameStatus(_gameStatus * ((board.sideToMove==ourColor)*2-1));}

    assert(moveList.size());
    chess::Move move = moveList[rand() % moveList.size()];
    //std::cout << ((chess::PieceToLetter(board.findPiece(move.getStartSquare()))=='P') ? ' ' : chess::PieceToLetter(board.findPiece(move.getStartSquare()))) << squareIndexToNotation(move.getStartSquare()) << squareIndexToNotation(move.getEndSquare()) << ((move.getMoveFlags() == chess::PROMOTION) ? chess::PieceToLetter(move.getPromotionPiece()) : ' ') << " ";
    board.makeMove(move);
    //board.printBoard();}
    //std::cout << "\n";
  }
}

struct Node{
  Node* parent;
  uint8_t index;
  std::unique_ptr<Node> firstChild;
  std::unique_ptr<Node> nextSibling;
  uint32_t visits;
  double value;

  Node(Node* parent, uint8_t index) :
  parent(parent), index(index),
  firstChild(nullptr), nextSibling(nullptr),
  visits(0), value(0) {}


};

}//namespace search