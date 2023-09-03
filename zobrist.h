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
    chess::Pieces piece = board.findPiece(i);
    if(piece){
      hash ^= pieceKeys[2*(piece-1)+(1ULL << i & board.white ? 0 : 1)][i];
    }
  }

  if(board.enPassant){
    hash ^= enPassantKeys[squareIndexToFile(_bitscanForward(board.enPassant))];
  }
  
  hash ^= castlingKeys[board.castlingRights];
  
  hash ^= board.sideToMove ? sideToMoveKey : 0ULL;

  board.hashed = true;
  return hash;
}
U64 updateHash(chess::Board& board, chess::Move move){
  U64 hash = board.history[board.halfmoveClock];
  const uint8_t startSquare = move.getStartSquare();
  const uint8_t endSquare = move.getEndSquare();
  const chess::Pieces movingPiece = board.findPiece(startSquare);
  const chess::MoveFlags moveFlags = move.getMoveFlags();

  hash ^= pieceKeys[2*(movingPiece-1)+(board.sideToMove ? 1 : 0)][startSquare];

  if(moveFlags == chess::ENPASSANT){
    hash ^= pieceKeys[2*(chess::PAWN-1)+(board.sideToMove ? 0 : 1)][(board.sideToMove ? endSquare + 8 : endSquare - 8)];
  }
  else{
    if(board.getTheirPieces() & (1ULL << endSquare)){
      hash ^= pieceKeys[2*(board.findPiece(endSquare)-1)+(board.sideToMove ? 0 : 1)][endSquare];
    }
  }

  if(moveFlags == chess::CASTLE){
    uint8_t rookStartSquare;
    uint8_t rookEndSquare;
    //Queenside Castling
    if(squareIndexToFile(endSquare) == 2){
      rookStartSquare = board.sideToMove*56;
      rookEndSquare = 3+board.sideToMove*56;
    }
    //Kingside Castling
    else{
      rookStartSquare = 7+board.sideToMove*56;
      rookEndSquare = 5+board.sideToMove*56;
    }
    hash ^= pieceKeys[2*(chess::ROOK-1)+(board.sideToMove ? 1 : 0)][rookStartSquare];

    hash ^= pieceKeys[2*(chess::ROOK-1)+(board.sideToMove ? 1 : 0)][rookEndSquare];
  }

  if(moveFlags == chess::PROMOTION){hash ^= pieceKeys[2*(move.getPromotionPiece()-1)+(board.sideToMove ? 1 : 0)][endSquare];}
  else{hash ^= pieceKeys[2*(movingPiece-1)+(board.sideToMove ? 1 : 0)][endSquare];}

  if(board.enPassant){
    hash ^= enPassantKeys[squareIndexToFile(_bitscanForward(board.enPassant))]; //remove en passant from hash
  }
  if(movingPiece == chess::PAWN){
    //double pawn push by white
    if((1ULL << endSquare) == (1ULL << startSquare) << 16){
      hash ^= enPassantKeys[squareIndexToFile(endSquare)];
    }
    //double pawn push by black
    else if((1ULL << endSquare) == (1ULL << startSquare) >> 16){
      hash ^= enPassantKeys[squareIndexToFile(endSquare)];
    }
  }
  bool castlingRightsChanged = false;
  //Remove castling rights if king moved
  if(movingPiece == chess::KING){
    if(board.sideToMove == chess::WHITE && board.castlingRights & (0x1 | 0x2)){
      hash ^= castlingKeys[board.castlingRights & ~(0x1 | 0x2)];
      castlingRightsChanged = true;
    }
    else if(board.castlingRights & (0x4 | 0x8)){
      hash ^= castlingKeys[board.castlingRights & ~(0x4 | 0x8)];
      castlingRightsChanged = true;
    }
  }
  //Remove castling rights if rook moved from starting square or if rook was captured
  if(((startSquare == 0 && movingPiece == chess::ROOK) || endSquare == 0) && board.castlingRights & ~0x2){hash ^= castlingKeys[board.castlingRights & ~0x2]; castlingRightsChanged = true;}
  if(((startSquare == 7 && movingPiece == chess::ROOK) || endSquare == 7) && board.castlingRights & ~0x1){hash ^= castlingKeys[board.castlingRights & ~0x1]; castlingRightsChanged = true;}
  if(((startSquare == 56 && movingPiece == chess::ROOK) || endSquare == 56) && board.castlingRights & ~0x8){hash ^= castlingKeys[board.castlingRights & ~0x8]; castlingRightsChanged = true;}
  if(((startSquare == 63 && movingPiece == chess::ROOK) || endSquare == 63) && board.castlingRights & ~0x4){hash ^= castlingKeys[board.castlingRights & ~0x4]; castlingRightsChanged = true;}

  if(castlingRightsChanged){hash ^= castlingKeys[board.castlingRights];} //remove original castling rights if castling rights changed

  hash ^= sideToMoveKey;

  return hash;
}
}//namespace
namespace chess{
  //The normal Board.makeMove except we update the zobrist hash. Use this rather than Board.makeMove for making moves during a game
  void makeMove(chess::Board& board, chess::Move move){
    if(board.hashed){
      U64 newHash = zobrist::updateHash(board, move);
      board.makeMove(move);
      board.history[board.halfmoveClock] = newHash;
    }
    else{
      board.makeMove(move);
      board.history[board.halfmoveClock] = zobrist::getHash(board);
    }
  }
}
