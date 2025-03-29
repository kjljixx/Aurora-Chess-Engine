#include "search.h"

namespace datagen{
void genFens(int numFens, uint64_t seed, int depth, std::vector<std::string> book = {chess::startPosFen}){
  std::mt19937_64 eng(seed);
  std::uniform_int_distribution<> bookDistr(0, book.size()-1);

  for(int i=0; i<numFens; i++){
    chess::Board board;
    do{
      std::string startFen = book[bookDistr(eng)];
      board.setToFen(startFen);
      
      for(int j=0; j<depth; j++){
        chess::MoveList moves(board);
        if(moves.size() == 0){break;}

        std::uniform_int_distribution<> moveDistr(0, moves.size()-1); 
        chess::makeMove(board, moves[moveDistr(eng)]);
      }
    } while(chess::getGameStatus(board, chess::isLegalMoves(board)) != chess::ONGOING);

    std::cout << "info string genfens " << board.getFen() << std::endl;
  }
}
}