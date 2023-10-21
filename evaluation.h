#include "zobrist.h"
#include <math.h>
#include <algorithm>
#include <random>

namespace evaluation{
int piecePairTable[64][13][64][13];

void init(){
  piecePairTable[0][4][28][1] = 1;
  piecePairTable[28][1][0][4] = 1;
}

struct Eval{
  int whiteToMove;
  int blackToMove;

  Eval() : whiteToMove(0), blackToMove(0) {}

  Eval(int white, int black) : whiteToMove(white), blackToMove(black) {}
};

Eval evaluate(chess::Board& board){
  Eval evaluation;

  U64 whitePieces = board.getPieces(chess::WHITE);
  U64 blackPieces = board.getPieces(chess::BLACK);
  for(int i=0; i<63; i++){
    for(int j=i+1; j<64; j++){
      int pieceI;
      int pieceJ;
      if(blackPieces & (1ULL << i)){pieceI = board.findPiece(i) + 6;}
      else{pieceI = board.findPiece(i);}
      if(blackPieces & (1ULL << j)){pieceJ = board.findPiece(j) + 6;}
      else{pieceJ = board.findPiece(j);}

      evaluation.whiteToMove += piecePairTable[i][pieceI][j][pieceJ];

      if(pieceI >= 7){pieceI -= 6;}
      else if(pieceI >= 1){pieceI += 6;}
      if(pieceJ >= 7){pieceJ -= 6;}
      else if(pieceJ >= 1){pieceJ += 6;}
      evaluation.blackToMove += piecePairTable[i^56][pieceI][j^56][pieceJ];
    }
  }

  return evaluation;
}

U64 counter;

 void fetchPiecePairTableValue(int a){
   counter+=a;
 }

Eval updateEvalOnSquare(Eval prevEval, chess::Board& board, uint8_t square, chess::Pieces newPieceType, chess::Colors newPieceColor = chess::WHITE){
  int changingPiece = board.mailbox[square];

  int newPiece = (newPieceColor == chess::WHITE) || (newPieceType == chess::null) ? newPieceType : newPieceType+6;
  
  int currPiece;

  for(int i=0; i<square; i++){
    currPiece = board.mailbox[i];
    
    prevEval.whiteToMove -= piecePairTable[square][changingPiece][i][currPiece];
    prevEval.whiteToMove += piecePairTable[square][newPiece][i][currPiece];
  }
  for(int i=square+1; i<64; i++){
    currPiece = board.mailbox[i];
    
    prevEval.whiteToMove -= piecePairTable[square][changingPiece][i][currPiece];
    prevEval.whiteToMove += piecePairTable[square][newPiece][i][currPiece];
  }

  if(changingPiece >= 7){changingPiece -= 6;}
  else if(changingPiece >= 1){changingPiece += 6;}

  if(newPiece >= 7){newPiece -= 6;}
  else if(newPiece >= 1){newPiece += 6;}

  square ^= 56;

  for(int i=0; i<square; i++){
    currPiece = board.mailbox[i^56];
    if(currPiece >= 7){currPiece -= 6;}
    else if(currPiece >= 1){currPiece += 6;}
    
    prevEval.blackToMove -= piecePairTable[square][changingPiece][i][currPiece];
    prevEval.blackToMove += piecePairTable[square][newPiece][i][currPiece];
  }
  for(int i=square+1; i<64; i++){
    currPiece = board.mailbox[i^56];
    if(currPiece >= 7){currPiece -= 6;}
    else if(currPiece >= 1){currPiece += 6;}
    
    prevEval.blackToMove -= piecePairTable[square][changingPiece][i][currPiece];
    prevEval.blackToMove += piecePairTable[square][newPiece][i][currPiece];
  }

  return prevEval;
}

Eval updateEvalOnMove(Eval prevEval, chess::Board& board, chess::Move move){
  U64 newHash;
  if(board.hashed){
    newHash = zobrist::updateHash(board, move);
  }

  board.halfmoveClock++;
  const uint8_t startSquare = move.getStartSquare();
  const uint8_t endSquare = move.getEndSquare();
  const chess::Pieces movingPiece = board.findPiece(startSquare);
  const chess::MoveFlags moveFlags = move.getMoveFlags();

  prevEval = updateEvalOnSquare(prevEval, board, startSquare, chess::null);
  board.mailbox[startSquare] = 0;
  board.unsetColors((1ULL << startSquare), board.sideToMove);
  board.unsetPieces(movingPiece, (1ULL << startSquare));

  if(moveFlags == chess::ENPASSANT){
    U64 theirPawnSquare;
    if(board.sideToMove == chess::WHITE){theirPawnSquare = (1ULL << endSquare) >> 8;}
    else{theirPawnSquare = (1ULL << endSquare) << 8;}

    uint8_t theirPawnSq = _bitscanForward(theirPawnSquare);

    prevEval = updateEvalOnSquare(prevEval, board, theirPawnSq, chess::null);
    board.mailbox[theirPawnSq] = 0;
    board.unsetColors(theirPawnSquare, chess::Colors(!board.sideToMove));
    board.unsetPieces(chess::PAWN, theirPawnSquare);
  }
  else{
    if(board.getTheirPieces() & (1ULL << endSquare)){
      board.halfmoveClock = 0;
      board.startHistoryIndex = 0;

      prevEval = updateEvalOnSquare(prevEval, board, endSquare, chess::null);
      board.mailbox[endSquare] = 0;
      board.unsetColors((1ULL << endSquare), chess::Colors(!board.sideToMove));
      board.unsetPieces(chess::UNKNOWN, (1ULL << endSquare));
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

    prevEval = updateEvalOnSquare(prevEval, board, rookStartSquare, chess::null);
    board.mailbox[rookStartSquare] = 0;
    board.unsetColors(rookStartSquare, board.sideToMove);
    board.unsetPieces(chess::ROOK, rookStartSquare);

    prevEval = updateEvalOnSquare(prevEval, board, rookEndSquare, chess::ROOK, board.sideToMove);
    board.mailbox[rookEndSquare] = board.sideToMove ? 10 : 4;
    board.setColors(rookEndSquare, board.sideToMove);
    board.setPieces(chess::ROOK, rookEndSquare);
  }

  if(moveFlags == chess::PROMOTION){
    prevEval = updateEvalOnSquare(prevEval, board, endSquare, move.getPromotionPiece(), board.sideToMove);
    board.mailbox[endSquare] = board.sideToMove ? move.getPromotionPiece()+6 : move.getPromotionPiece();
    board.setPieces(move.getPromotionPiece(), (1ULL << endSquare));
  }
  else{
    prevEval = updateEvalOnSquare(prevEval, board, endSquare, movingPiece, board.sideToMove);
    board.mailbox[endSquare] = movingPiece;
    board.setPieces(movingPiece, (1ULL << endSquare));
  }
  board.setColors((1ULL << endSquare), board.sideToMove);

  board.enPassant = 0ULL;
  if(movingPiece == chess::PAWN){
    board.halfmoveClock = 0;
    board.startHistoryIndex = 0;
    board.enPassant = 0ULL;

    //double pawn push by white
    if((1ULL << endSquare) == (1ULL << startSquare) << 16){
      board.enPassant = (1ULL << startSquare) << 8;
    }
    //double pawn push by black
    else if((1ULL << endSquare) == (1ULL << startSquare) >> 16){
      board.enPassant = (1ULL << startSquare) >> 8;
    }
  }
  //Remove castling rights if king moved
  if(movingPiece == chess::KING){
    if(board.sideToMove == chess::WHITE){
      board.castlingRights &= ~(0x1 | 0x2);
    }
    else{
      board.castlingRights &= ~(0x4 | 0x8);
    }
  }
  //Remove castling rights if rook moved from starting square or if rook was captured
  if((startSquare == 0 && movingPiece == chess::ROOK) || endSquare == 0){board.castlingRights &= ~0x2;}
  if((startSquare == 7 && movingPiece == chess::ROOK) || endSquare == 7){board.castlingRights &= ~0x1;}
  if((startSquare == 56 && movingPiece == chess::ROOK) || endSquare == 56){board.castlingRights &= ~0x8;}
  if((startSquare == 63 && movingPiece == chess::ROOK) || endSquare == 63){board.castlingRights &= ~0x4;}
  
  board.occupied = board.white | board.black;
  board.sideToMove = chess::Colors(!board.sideToMove);

  if(board.hashed){
    board.history[board.halfmoveClock] = newHash;
  }
  else{
    board.history[board.halfmoveClock] = zobrist::getHash(board);
  }

  return prevEval;
}
}