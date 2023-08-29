#include "chess.h"

namespace zobrist{
U64 seed = 1070372;
//From Stockfish
uint64_t random_U64() {
	seed ^= seed >> 12;
	seed ^= seed << 25;
	seed ^= seed >> 27;
	seed *= UINT64_C(2685821657736338717);
	return seed;
}

U64 pieceKeys[12][64];
U64 sideToMoveKey;
U64 castlingKeys[16];
U64 enPassantKeys[8];

void init(){
  for(int i=0; i<12; i++){
    for(int j=0; j<64; j++){
      pieceKeys[i][j] = random_U64();
    }
  }
  sideToMoveKey = random_U64();
  for(int i=0; i<16; i++){
    castlingKeys[i] = random_U64();
  }
  for(int i=0; i<8; i++){
    enPassantKeys[i] = random_U64();
  }
}

U64 getHash(chess::Board& board){
  U64 hash = 0ULL;

  for(int i=0; i<64; i++){
    hash ^= pieceKeys[board.findPiece(i)][i];
  }

  if(board.enPassant){
    hash ^= enPassantKeys[squareIndexToFile(_bitscanForward(board.enPassant))];
  }
  
  hash ^= castlingKeys[board.castlingRights];
  
  hash ^= board.sideToMove;

  return hash;
}
}//namespace
namespace chess{
  void makeMove(chess::Board& board, chess::Move move){
    board.makeMove(move);
    board.history[board.halfmoveClock] = zobrist::getHash(board);
  }
}
