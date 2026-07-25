#pragma once
#include "chess.h"

namespace zobrist{
inline U64 seed = 1070372;
//From Stockfish
inline U64 random_U64() {
	seed ^= seed >> 12;
	seed ^= seed << 25;
	seed ^= seed >> 27;
	seed *= UINT64_C(2685821657736338717);
	return seed;
}

inline bool shouldHashEnPassant(chess::Board& board){
  if(!board.enPassant){
    return false;
  }

  const uint8_t enPassantSquare = bitscanForward(board.enPassant);
  const U64 enPassantCapturers = lookupTables::pawnAttackTable[!board.sideToMove][enPassantSquare] & board.getOurPieces(chess::PAWN);
  return enPassantCapturers != 0ULL;
}

inline U64 pieceKeys[12][64];
inline U64 sideToMoveKey;
inline U64 castlingKeys[16];
inline U64 enPassantKeys[8];

inline void init(){
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

inline U64 getHash(chess::Board& board){
  U64 hash = 0ULL;

  for(int i=0; i<64; i++){
    chess::Pieces piece = board.findPiece(i);
    if(piece){
      hash ^= pieceKeys[2*(piece-1)+(1ULL << i & board.white ? 0 : 1)][i];
    }
  }

  if(shouldHashEnPassant(board)){
    hash ^= enPassantKeys[squareIndexToFile(bitscanForward(board.enPassant))];
  }
  
  hash ^= castlingKeys[board.castlingRights];
  
  hash ^= board.sideToMove ? sideToMoveKey : 0ULL;

  return hash;
}
inline U64 updateHash(chess::Board& board, chess::Move move){
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
    uint8_t rookStartSquare = 0;
    uint8_t rookEndSquare = 0;
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

  if(shouldHashEnPassant(board)){
    hash ^= enPassantKeys[squareIndexToFile(bitscanForward(board.enPassant))]; //remove en passant from hash
  }
  if(movingPiece == chess::PAWN){
    U64 newEnPassant = 0ULL;
    //double pawn push by white
    if((1ULL << endSquare) == (1ULL << startSquare) << 16){
      newEnPassant = (1ULL << startSquare) << 8;
    }
    //double pawn push by black
    else if((1ULL << endSquare) == (1ULL << startSquare) >> 16){
      newEnPassant = (1ULL << startSquare) >> 8;
    }

    if(newEnPassant){
      const uint8_t enPassantSquare = bitscanForward(newEnPassant);
      const U64 theirPawns = board.getTheirPieces(chess::PAWN);
      const U64 enPassantCapturers = lookupTables::pawnAttackTable[board.sideToMove][enPassantSquare] & theirPawns;
      if(enPassantCapturers){
        hash ^= enPassantKeys[squareIndexToFile(enPassantSquare)];
      }
    }
  }
  // Compute final castling rights once. A single move can clear multiple rights
  // (e.g. Ra1xa8), so intermediate keys must not be XOR'd individually.
  unsigned char newCastlingRights = board.castlingRights;
  if(movingPiece == chess::KING){
    if(board.sideToMove == chess::WHITE){newCastlingRights &= ~(0x1 | 0x2);}
    else{newCastlingRights &= ~(0x4 | 0x8);}
  }
  if((startSquare == 0 && movingPiece == chess::ROOK) || endSquare == 0){newCastlingRights &= ~0x2;}
  if((startSquare == 7 && movingPiece == chess::ROOK) || endSquare == 7){newCastlingRights &= ~0x1;}
  if((startSquare == 56 && movingPiece == chess::ROOK) || endSquare == 56){newCastlingRights &= ~0x8;}
  if((startSquare == 63 && movingPiece == chess::ROOK) || endSquare == 63){newCastlingRights &= ~0x4;}

  if(newCastlingRights != board.castlingRights){
    hash ^= castlingKeys[board.castlingRights];
    hash ^= castlingKeys[newCastlingRights];
  }

  hash ^= sideToMoveKey;

  return hash;
}
}//namespace
namespace chess{
  //The normal Board.makeMove except we update the zobrist hash. Use this rather than Board.makeMove for making moves during a game
  inline void makeMove(chess::Board& board, chess::Move move){
    if(!board.hashed){
      // Seed history with the current position so repetition counting includes the root.
      board.history[board.halfmoveClock] = zobrist::getHash(board);
      board.hashed = true;
    }

    U64 newHash = zobrist::updateHash(board, move);
    board.makeMove(move);
    board.history[board.halfmoveClock] = newHash;
  }
}
