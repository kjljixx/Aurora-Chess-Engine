#include "zobrist.h"
#include <math.h>
#include <algorithm>
#include <random>

namespace evaluation{
int piecePairTable[2016][169];

void init(){
  for(int i=0; i<2016; i++){
    for(int j=0; j<169; j++){
      piecePairTable[i][j] = (rand() % 2)*2-1;
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
int squarePairToIndex(uint8_t squareA, uint8_t squareB, bool compare){
  if(compare){
    if(squareA > squareB){
      return squareB*(125-squareB)/2+squareA-1;
    }
  }
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

      evaluation.whiteToMove += piecePairTable[index][piecePair];
      index++;
    }
  }
  for(int i=0; i<63; i++){
    for(int j=i+1; j<64; j++){
      int piecePair = 0;
      if(whitePieces & (1ULL << i)){piecePair += board.findPiece(i) + 6;}
      else{piecePair += board.findPiece(i);}
      if(whitePieces & (1ULL << j)){piecePair += 13*(board.findPiece(j) + 6);}
      else{piecePair += 13*board.findPiece(j);}

      evaluation.blackToMove += piecePairTable[squarePairToIndex(i^56, j^56, true)][piecePair];
    }
  }

  return evaluation;
}

Eval updateEvalOnSquare(Eval prevEval, chess::Board& board, uint8_t square, chess::Pieces newPieceType, chess::Colors newPieceColor){
  U64 whitePieces = board.getPieces(chess::WHITE);
  U64 blackPieces = board.getPieces(chess::BLACK);

  int changingPiece = board.findPiece(square);
  if(blackPieces & (1ULL << square)){changingPiece += 6;}

  int newPiece = (newPieceColor == chess::WHITE) || (newPieceType == chess::null) ? newPieceType : newPieceType+6;

  for(int i=0; i<square; i++){
    int currPiece = board.findPiece(i);
    if(blackPieces & (1ULL << i)){currPiece += 6;}

    int squarePairIndex = squarePairToIndex(i, square, false);

    prevEval.whiteToMove -= piecePairTable[squarePairIndex][(13*changingPiece)+(currPiece)];
    prevEval.whiteToMove += piecePairTable[squarePairIndex][(13*newPiece)+(currPiece)];
  }
  for(int i=square+1; i<64; i++){
    int currPiece = board.findPiece(i);
    if(blackPieces & (1ULL << i)){currPiece += 6;}

    int squarePairIndex = squarePairToIndex(square, i, false);

    prevEval.whiteToMove -= piecePairTable[squarePairIndex][(13*currPiece)+(changingPiece)];
    prevEval.whiteToMove += piecePairTable[squarePairIndex][(13*currPiece)+(newPiece)];
  }

  if(changingPiece >= 7){changingPiece -= 6;}
  else if(changingPiece >= 1){changingPiece += 6;}

  if(newPiece >= 7){newPiece -= 6;}
  else if(newPiece >= 1){newPiece += 6;}

  for(int i=0; i<square; i++){
    int currPiece = board.findPiece(i);
    if(whitePieces & (1ULL << i)){currPiece += 6;}

    int squarePairIndex = squarePairToIndex(i^56, square^56, true);

    prevEval.blackToMove -= piecePairTable[squarePairIndex][(13*changingPiece)+(currPiece)];
    prevEval.blackToMove += piecePairTable[squarePairIndex][(13*newPiece)+(currPiece)];
  }
  for(int i=(square+1); i<64; i++){
    int currPiece = board.findPiece(i);
    if(whitePieces & (1ULL << i)){currPiece += 6;}

    int squarePairIndex = squarePairToIndex(square^56, i^56, true);

    prevEval.blackToMove -= piecePairTable[squarePairIndex][(13*currPiece)+(changingPiece)];
    prevEval.blackToMove += piecePairTable[squarePairIndex][(13*currPiece)+(newPiece)];
  }

  return prevEval;
}
}