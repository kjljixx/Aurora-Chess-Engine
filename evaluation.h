#include "zobrist.h"
#include <math.h>
#include <algorithm>
#include <random>

namespace evaluation{
//Evaluation Parameters
int evalStabilityConstant = 10;

//Taken from PeSTO: https://www.chessprogramming.org/PeSTO%27s_Evaluation_Function
int mg_value[6] = { 82, 337, 365, 477, 1025,  0};
int eg_value[6] = { 94, 281, 297, 512,  936,  0};

int mg_pawn_table[64] = {
      0,   0,   0,   0,   0,   0,  0,   0,
     98, 134,  61,  95,  68, 126, 34, -11,
     -6,   7,  26,  31,  65,  56, 25, -20,
    -14,  13,   6,  21,  23,  12, 17, -23,
    -27,  -2,  -5,  12,  17,   6, 10, -25,
    -26,  -4,  -4, -10,   3,   3, 33, -12,
    -35,  -1, -20, -23, -15,  24, 38, -22,
      0,   0,   0,   0,   0,   0,  0,   0,
};

int eg_pawn_table[64] = {
      0,   0,   0,   0,   0,   0,   0,   0,
    178, 173, 158, 134, 147, 132, 165, 187,
     94, 100,  85,  67,  56,  53,  82,  84,
     32,  24,  13,   5,  -2,   4,  17,  17,
     13,   9,  -3,  -7,  -7,  -8,   3,  -1,
      4,   7,  -6,   1,   0,  -5,  -1,  -8,
     13,   8,   8,  10,  13,   0,   2,  -7,
      0,   0,   0,   0,   0,   0,   0,   0,
};

int mg_knight_table[64] = {
    -167, -89, -34, -49,  61, -97, -15, -107,
     -73, -41,  72,  36,  23,  62,   7,  -17,
     -47,  60,  37,  65,  84, 129,  73,   44,
      -9,  17,  19,  53,  37,  69,  18,   22,
     -13,   4,  16,  13,  28,  19,  21,   -8,
     -23,  -9,  12,  10,  19,  17,  25,  -16,
     -29, -53, -12,  -3,  -1,  18, -14,  -19,
    -105, -21, -58, -33, -17, -28, -19,  -23,
};

int eg_knight_table[64] = {
    -58, -38, -13, -28, -31, -27, -63, -99,
    -25,  -8, -25,  -2,  -9, -25, -24, -52,
    -24, -20,  10,   9,  -1,  -9, -19, -41,
    -17,   3,  22,  22,  22,  11,   8, -18,
    -18,  -6,  16,  25,  16,  17,   4, -18,
    -23,  -3,  -1,  15,  10,  -3, -20, -22,
    -42, -20, -10,  -5,  -2, -20, -23, -44,
    -29, -51, -23, -15, -22, -18, -50, -64,
};

int mg_bishop_table[64] = {
    -29,   4, -82, -37, -25, -42,   7,  -8,
    -26,  16, -18, -13,  30,  59,  18, -47,
    -16,  37,  43,  40,  35,  50,  37,  -2,
     -4,   5,  19,  50,  37,  37,   7,  -2,
     -6,  13,  13,  26,  34,  12,  10,   4,
      0,  15,  15,  15,  14,  27,  18,  10,
      4,  15,  16,   0,   7,  21,  33,   1,
    -33,  -3, -14, -21, -13, -12, -39, -21,
};

int eg_bishop_table[64] = {
    -14, -21, -11,  -8, -7,  -9, -17, -24,
     -8,  -4,   7, -12, -3, -13,  -4, -14,
      2,  -8,   0,  -1, -2,   6,   0,   4,
     -3,   9,  12,   9, 14,  10,   3,   2,
     -6,   3,  13,  19,  7,  10,  -3,  -9,
    -12,  -3,   8,  10, 13,   3,  -7, -15,
    -14, -18,  -7,  -1,  4,  -9, -15, -27,
    -23,  -9, -23,  -5, -9, -16,  -5, -17,
};

int mg_rook_table[64] = {
     32,  42,  32,  51, 63,  9,  31,  43,
     27,  32,  58,  62, 80, 67,  26,  44,
     -5,  19,  26,  36, 17, 45,  61,  16,
    -24, -11,   7,  26, 24, 35,  -8, -20,
    -36, -26, -12,  -1,  9, -7,   6, -23,
    -45, -25, -16, -17,  3,  0,  -5, -33,
    -44, -16, -20,  -9, -1, 11,  -6, -71,
    -19, -13,   1,  17, 16,  7, -37, -26,
};

int eg_rook_table[64] = {
    13, 10, 18, 15, 12,  12,   8,   5,
    11, 13, 13, 11, -3,   3,   8,   3,
     7,  7,  7,  5,  4,  -3,  -5,  -3,
     4,  3, 13,  1,  2,   1,  -1,   2,
     3,  5,  8,  4, -5,  -6,  -8, -11,
    -4,  0, -5, -1, -7, -12,  -8, -16,
    -6, -6,  0,  2, -9,  -9, -11,  -3,
    -9,  2,  3, -1, -5, -13,   4, -20,
};

int mg_queen_table[64] = {
    -28,   0,  29,  12,  59,  44,  43,  45,
    -24, -39,  -5,   1, -16,  57,  28,  54,
    -13, -17,   7,   8,  29,  56,  47,  57,
    -27, -27, -16, -16,  -1,  17,  -2,   1,
     -9, -26,  -9, -10,  -2,  -4,   3,  -3,
    -14,   2, -11,  -2,  -5,   2,  14,   5,
    -35,  -8,  11,   2,   8,  15,  -3,   1,
     -1, -18,  -9,  10, -15, -25, -31, -50,
};

int eg_queen_table[64] = {
     -9,  22,  22,  27,  27,  19,  10,  20,
    -17,  20,  32,  41,  58,  25,  30,   0,
    -20,   6,   9,  49,  47,  35,  19,   9,
      3,  22,  24,  45,  57,  40,  57,  36,
    -18,  28,  19,  47,  31,  34,  39,  23,
    -16, -27,  15,   6,   9,  17,  10,   5,
    -22, -23, -30, -16, -16, -23, -36, -32,
    -33, -28, -22, -43,  -5, -32, -20, -41,
};

int mg_king_table[64] = {
    -65,  23,  16, -15, -56, -34,   2,  13,
     29,  -1, -20,  -7,  -8,  -4, -38, -29,
     -9,  24,   2, -16, -20,   6,  22, -22,
    -17, -20, -12, -27, -30, -25, -14, -36,
    -49,  -1, -27, -39, -46, -44, -33, -51,
    -14, -14, -22, -46, -44, -30, -15, -27,
      1,   7,  -8, -64, -43, -16,   9,   8,
    -15,  36,  12, -54,   8, -28,  24,  14,
};

int eg_king_table[64] = {
    -74, -35, -18, -18, -11,  15,   4, -17,
    -12,  17,  14,  17,  17,  38,  23,  11,
     10,  17,  23,  15,  20,  45,  44,  13,
     -8,  22,  24,  27,  26,  33,  26,   3,
    -18,  -4,  21,  24,  27,  23,   9, -11,
    -19,  -3,  11,  21,  23,  16,   7,  -9,
    -27, -11,   4,  13,  14,   4,  -5, -17,
    -53, -34, -21, -11, -28, -14, -24, -43
};

int* mg_table[6] =
{
    mg_pawn_table,
    mg_knight_table,
    mg_bishop_table,
    mg_rook_table,
    mg_queen_table,
    mg_king_table
};

int* eg_table[6] =
{
    eg_pawn_table,
    eg_knight_table,
    eg_bishop_table,
    eg_rook_table,
    eg_queen_table,
    eg_king_table
};

int gamephaseInc[6] = {0, 1, 1, 2, 4, 0};

int gamePhase = 4;

int currentPieceSquareTableEval = 0;

int pieceSquareTable(chess::Board& board){
  int mg[2];
  int eg[2];
  gamePhase = 0;

  mg[chess::WHITE] = 0;
  mg[chess::BLACK] = 0;
  eg[chess::WHITE] = 0;
  eg[chess::BLACK] = 0;

  /* evaluate each piece */
  for (int sq = 0; sq < 64; ++sq) {
    if(!((1ULL << sq) & board.occupied)){continue;}
    chess::Pieces pc = board.findPiece(sq);
    if (pc != chess::null) {
      if((1ULL << sq) & board.black){
        mg[1] += mg_table[pc-1][sq] + mg_value[pc-1];
        eg[1] += eg_table[pc-1][sq] + mg_value[pc-1];
      }
      else{
        mg[0] += mg_table[pc-1][sq^56] + mg_value[pc-1];
        eg[0] += eg_table[pc-1][sq^56] + mg_value[pc-1];
      }
      gamePhase += gamephaseInc[pc-1];
    }
  }

  /* tapered eval */
  int mgScore = mg[board.sideToMove] - mg[!board.sideToMove];
  int egScore = eg[board.sideToMove] - eg[!board.sideToMove];

  if (gamePhase > 24){gamePhase = 24;} /* in case of early promotion */
  int mgPhase = gamePhase;
  int egPhase = 24 - mgPhase;

  currentPieceSquareTableEval = (mgScore * mgPhase + egScore * egPhase) / 24;
  return currentPieceSquareTableEval;
}

//Static Exchange Evaluation
int SEE(chess::Board& board, uint8_t lastMoveEndSquare){
  int values[32];
  int i=0;

  chess::Pieces currPiece = board.findPiece(lastMoveEndSquare);
  values[i] = (mg_value[currPiece-1] * gamePhase + eg_value[currPiece-1] * (24-gamePhase))/24;

  chess::Colors us = board.sideToMove;
  U64 white = board.white;
  U64 black = board.black;

  board.sideToMove = chess::Colors(!board.sideToMove);
  bool isOurSideToMove = false;

  uint8_t piecePos = board.squareUnderAttack(lastMoveEndSquare);
  chess::Pieces leastValuableAttacker = board.findPiece(piecePos);

  while(leastValuableAttacker){
    i++;
    values[i] = (mg_value[leastValuableAttacker-1] * gamePhase + eg_value[leastValuableAttacker-1] * (24-gamePhase))/24 - values[i-1];

    if(std::max(-values[i-1], values[i]) < 0){break;}

    board.sideToMove = chess::Colors(!board.sideToMove); isOurSideToMove = !isOurSideToMove;
    board.unsetColors(1ULL << piecePos, chess::Colors(isOurSideToMove ? us : !us));

    piecePos = board.squareUnderAttack(lastMoveEndSquare);
    leastValuableAttacker = board.findPiece(piecePos);
  }

  board.sideToMove = us;
  board.white = white;
  board.black = black;

  if(i == 0){
    return 0;
  }

  while(--i > 0){
    values[i-1] = -std::max(-values[i-1], values[i]);
  }


  return values[0];
}

float offset = 0;

float randomOffset(){
  offset >= 0.1 ? offset = 0 : offset += 0.00001;
  return offset;
}

float evaluate(chess::Board& board, chess::Move lastMove = chess::Move(), float previousEval = 0){
  int cpEvaluation = 0;

  //Piece Square Table Eval
  cpEvaluation += pieceSquareTable(board);

  //Static Exchange Eval
  int maxSEE = 0;
  U64 theirPieces = board.getTheirPieces();
  for(int i=0; i<64; i++){
    if((1ULL << i) & ~theirPieces){continue;}
    maxSEE = std::max(maxSEE, SEE(board, i));
  }
  cpEvaluation += maxSEE;

  cpEvaluation += evalStabilityConstant; //bias the eval to stabilize when searching
  return fmax(fmin(atan(cpEvaluation/100.0)/1.56375, 1),-1)*0.999999; //convert cp eval to wdl for ucb algorithm. Multiply by 0.999 to make sure definite wins (checkmates) are prioritised.
}
}