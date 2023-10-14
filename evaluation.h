#include "zobrist.h"
#include <math.h>
#include <algorithm>
#include <random>

namespace evaluation{
int piecePairTable[64][64][169];

void init(){
  for(int i=0; i<64; i++){
    for(int j=0; j<64; j++){
      for(int k=0; k<64; k++){
        piecePairTable[i][j][k] = 0;
        if(i==0 && j==28 && k==17){piecePairTable[i][j][k] = 1;}
      }
    }
  }
}

struct Eval{
  int whiteToMove;
  int blackToMove;

  Eval() : whiteToMove(0), blackToMove(0) {}

  Eval(int white, int black) : whiteToMove(white), blackToMove(black) {}
};

//Square B must be greater than square A
int squarePairToIndex(uint8_t squareA, uint8_t squareB){
  return squareA*(125-squareA)/2+squareB-1;
}

Eval evaluate(chess::Board& board){
  Eval evaluation;

  U64 whitePieces = board.getPieces(chess::WHITE);
  U64 blackPieces = board.getPieces(chess::BLACK);
  int index = 0;
  for(int i=0; i<63; i++){
    for(int j=i+1; j<64; j++){
      int piecePair = 0;
      if(blackPieces & (1ULL << i)){piecePair += board.findPiece(i) + 6;}
      else{piecePair += board.findPiece(i);}
      if(blackPieces & (1ULL << j)){piecePair += 13*(board.findPiece(j) + 6);}
      else{piecePair += 13*board.findPiece(j);}

      evaluation.whiteToMove += piecePairTable[i][j][piecePair];
      index++;
    }
  }
  for(int i=0; i<63; i++){
    for(int j=i+1; j<64; j++){
      if((j^56) > (i^56)){
        int piecePair = 0;
        if(whitePieces & (1ULL << i)){piecePair += board.findPiece(i) + 6;}
        else{piecePair += board.findPiece(i);}
        if(whitePieces & (1ULL << j)){piecePair += 13*(board.findPiece(j) + 6);}
        else{piecePair += 13*board.findPiece(j);}

        evaluation.blackToMove += piecePairTable[i^56][j^56][piecePair];
      }
      else{
        int piecePair = 0;
        if(whitePieces & (1ULL << i)){piecePair += 13*(board.findPiece(i) + 6);}
        else{piecePair += 13*board.findPiece(i);}
        if(whitePieces & (1ULL << j)){piecePair += board.findPiece(j) + 6;}
        else{piecePair += board.findPiece(j);}

        evaluation.blackToMove += piecePairTable[j^56][i^56][piecePair];
      }
    }
  }

  return evaluation;
}

Eval updateEvalOnSquare(Eval prevEval, chess::Board& board, uint8_t square, chess::Pieces newPieceType, chess::Colors newPieceColor = chess::WHITE){
  U64 whitePieces = board.getPieces(chess::WHITE);
  U64 blackPieces = board.getPieces(chess::BLACK);

  int changingPiece = board.findPiece(square);
  if(blackPieces & (1ULL << square)){changingPiece += 6;}

  int newPiece = (newPieceColor == chess::WHITE) || (newPieceType == chess::null) ? newPieceType : newPieceType+6;

  U64 pieceBitboard = ~(board.white | board.black);

  while(pieceBitboard){
    uint8_t piecePos = _popLsb(pieceBitboard);

    if(piecePos > square){
      prevEval.whiteToMove -= piecePairTable[square][piecePos][changingPiece];
      prevEval.whiteToMove += piecePairTable[square][piecePos][newPiece];
    }
    else if(piecePos < square){
      prevEval.whiteToMove -= piecePairTable[piecePos][square][13*changingPiece];
      prevEval.whiteToMove += piecePairTable[piecePos][square][13*newPiece];
    }
  }

  for(int piece=chess::PAWN; piece<=chess::KING; piece++){
    U64 pieceBitboard = board.getPieces(chess::WHITE, chess::Pieces(piece));

    while(pieceBitboard){
      uint8_t piecePos = _popLsb(pieceBitboard);

      if(piecePos > square){
        prevEval.whiteToMove -= piecePairTable[square][piecePos][13*piece+changingPiece];
        prevEval.whiteToMove += piecePairTable[square][piecePos][13*piece+newPiece];
      }
      else if(piecePos < square){
        prevEval.whiteToMove -= piecePairTable[piecePos][square][piece+13*changingPiece];
        prevEval.whiteToMove += piecePairTable[piecePos][square][piece+13*newPiece];
      }
    }

    pieceBitboard = board.getPieces(chess::BLACK, chess::Pieces(piece));

    piece += 6;
    
    while(pieceBitboard){
      uint8_t piecePos = _popLsb(pieceBitboard);

      if(piecePos > square){
        prevEval.whiteToMove -= piecePairTable[square][piecePos][13*piece+changingPiece];
        prevEval.whiteToMove += piecePairTable[square][piecePos][13*piece+newPiece];
      }
      else if(piecePos < square){
        prevEval.whiteToMove -= piecePairTable[piecePos][square][piece+13*changingPiece];
        prevEval.whiteToMove += piecePairTable[piecePos][square][piece+13*newPiece];
      }
    }

    piece -=6;
  }

  if(changingPiece >= 7){changingPiece -= 6;}
  else if(changingPiece >= 1){changingPiece += 6;}

  if(newPiece >= 7){newPiece -= 6;}
  else if(newPiece >= 1){newPiece += 6;}

  square ^= 56;

  pieceBitboard = ~(board.white | board.black); _flipBoard(pieceBitboard);

  while(pieceBitboard){
    uint8_t piecePos = _popLsb(pieceBitboard);

    if(piecePos > square){
      prevEval.blackToMove -= piecePairTable[square][piecePos][changingPiece];
      prevEval.blackToMove += piecePairTable[square][piecePos][newPiece];
    }
    else if(piecePos < square){
      prevEval.blackToMove -= piecePairTable[piecePos][square][13*changingPiece];
      prevEval.blackToMove += piecePairTable[piecePos][square][13*newPiece];
    }
  }

  for(int piece=chess::PAWN; piece<=chess::KING; piece++){
    U64 pieceBitboard = board.getPieces(chess::WHITE, chess::Pieces(piece)); _flipBoard(pieceBitboard);

    while(pieceBitboard){
      uint8_t piecePos = _popLsb(pieceBitboard);

      if(piecePos > square){
        prevEval.blackToMove -= piecePairTable[square][piecePos][13*piece+changingPiece];
        prevEval.blackToMove += piecePairTable[square][piecePos][13*piece+newPiece];
      }
      else if(piecePos < square){
        prevEval.blackToMove -= piecePairTable[piecePos][square][piece+13*changingPiece];
        prevEval.blackToMove += piecePairTable[piecePos][square][piece+13*newPiece];
      }
    }

    pieceBitboard = board.getPieces(chess::BLACK, chess::Pieces(piece)); _flipBoard(pieceBitboard);

    piece += 6;
    
    while(pieceBitboard){
      uint8_t piecePos = _popLsb(pieceBitboard);

      if(piecePos > square){
        prevEval.blackToMove -= piecePairTable[square][piecePos][13*piece+changingPiece];
        prevEval.blackToMove += piecePairTable[square][piecePos][13*piece+newPiece];
      }
      else if(piecePos < square){
        prevEval.blackToMove -= piecePairTable[piecePos][square][piece+13*changingPiece];
        prevEval.blackToMove += piecePairTable[piecePos][square][piece+13*newPiece];
      }
    }

    piece -=6;
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
  board.unsetColors((1ULL << startSquare), board.sideToMove);
  board.unsetPieces(movingPiece, (1ULL << startSquare));

  if(moveFlags == chess::ENPASSANT){
    U64 theirPawnSquare;
    if(board.sideToMove == chess::WHITE){theirPawnSquare = (1ULL << endSquare) >> 8;}
    else{theirPawnSquare = (1ULL << endSquare) << 8;}

    prevEval = updateEvalOnSquare(prevEval, board, theirPawnSquare, chess::null);
    board.unsetColors(theirPawnSquare, chess::Colors(!board.sideToMove));
    board.unsetPieces(chess::PAWN, theirPawnSquare);
  }
  else{
    if(board.getTheirPieces() & (1ULL << endSquare)){
      board.halfmoveClock = 0;
      board.startHistoryIndex = 0;

      prevEval = updateEvalOnSquare(prevEval, board, endSquare, chess::null);
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
    board.unsetColors(rookStartSquare, board.sideToMove);
    board.unsetPieces(chess::ROOK, rookStartSquare);

    prevEval = updateEvalOnSquare(prevEval, board, rookEndSquare, chess::ROOK, board.sideToMove);
    board.setColors(rookEndSquare, board.sideToMove);
    board.setPieces(chess::ROOK, rookEndSquare);
  }

  if(moveFlags == chess::PROMOTION){
    prevEval = updateEvalOnSquare(prevEval, board, endSquare, move.getPromotionPiece(), board.sideToMove);
    board.setPieces(move.getPromotionPiece(), (1ULL << endSquare));
  }
  else{
    prevEval = updateEvalOnSquare(prevEval, board, endSquare, movingPiece, board.sideToMove);
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