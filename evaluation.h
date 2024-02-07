#pragma once
#include "zobrist.h"
#include <math.h>
#include <algorithm>
#include <random>
#include <array>
#include "external/incbin.h"

namespace evaluation{

int mg_value[6] = {42, 184, 207, 261, 642, 10000};
int eg_value[6] = {71, 242, 265, 538, 1067, 10000};

int mg_pawn_table[64] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  89, 98, 71, 89, 64, 75, -35, -10,
  62, 66, 81, 94, 113, 103, 62, 51,
  52, 73, 68, 83, 84, 73, 79, 47,
  42, 62, 60, 73, 75, 67, 75, 45,
  43, 60, 58, 54, 64, 61, 92, 56,
  36, 61, 43, 41, 50, 82, 96, 49,
  0, 0, 0, 0, 0, 0, 0, 0
};
int eg_pawn_table[64] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  193, 190, 167, 132, 149, 165, 210, 208,
  194, 197, 172, 147, 142, 154, 180, 183,
  137, 131, 117, 106, 102, 108, 121, 122,
  119, 117, 104, 100, 101, 102, 109, 106,
  111, 114, 103, 110, 111, 104, 107, 99,
  120, 116, 115, 110, 118, 109, 111, 102,
  0, 0, 0, 0, 0, 0, 0, 0
};
int mg_knight_table[64] = {
  128, 176, 214, 218, 259, 244, 218, 163,
  194, 197, 226, 204, 219, 249, 205, 194,
  183, 222, 210, 235, 239, 265, 225, 218,
  182, 197, 201, 217, 203, 230, 193, 205,
  177, 186, 195, 190, 200, 196, 198, 178, 
  161, 174, 187, 189, 195, 189, 200, 167,
  162, 147, 166, 176, 180, 191, 168, 173,
  129, 159, 157, 154, 163, 171, 158, 160
};
int eg_knight_table[64] = {
  274, 316, 327, 323, 314, 318, 326, 279,
  309, 332, 332, 352, 351, 327, 327, 331,
  315, 330, 352, 352, 346, 345, 338, 319,
  327, 346, 356, 363, 362, 355, 348, 324,
  323, 340, 358, 362, 355, 354, 344, 325,
  324, 338, 338, 352, 354, 338, 319, 319,
  303, 329, 333, 336, 334, 322, 327, 304,
  296, 298, 317, 326, 321, 314, 307, 273
};
int mg_bishop_table[64] = {
  193, 219, 199, 202, 215, 219, 246, 219,
  188, 220, 221, 221, 225, 256, 220, 217,
  209, 235, 234, 233, 234, 236, 233, 223,
  211, 210, 222, 244, 230, 229, 211, 215,
  208, 215, 213, 223, 229, 212, 215, 210,
  206, 215, 215, 214, 213, 222, 218, 218,
  206, 214, 216, 201, 209, 220, 230, 209,
  183, 203, 192, 195, 195, 190, 185, 198
};
int eg_bishop_table[64] = {
  355, 348, 352, 354, 355, 351, 344, 353,
  355, 353, 357, 352, 355, 349, 351, 343,
  351, 353, 355, 360, 353, 362, 356, 351,
  344, 364, 363, 363, 371, 364, 361, 352, 
  345, 359, 367, 374, 363, 365, 354, 344,
  344, 355, 364, 363, 369, 358, 348, 339,
  344, 340, 349, 356, 357, 348, 340, 327,
  342, 345, 337, 350, 349, 348, 347, 340
};
int mg_rook_table[64] = {
  305, 312, 299, 293, 305, 307, 326, 334,
  295, 302, 322, 329, 328, 341, 338, 334,
  278, 293, 297, 311, 306, 314, 323, 298,
  262, 271, 283, 297, 291, 296, 275, 269,
  252, 262, 267, 273, 277, 265, 280, 255,
  246, 253, 259, 260, 270, 268, 273, 250,
  245, 260, 256, 266, 268, 279, 269, 238,
  264, 267, 274, 283, 282, 275, 256, 263
};
int eg_rook_table[64] = {
  600, 600, 605, 606, 604, 603, 599, 593,
  605, 606, 601, 599, 596, 593, 594, 591,
  602, 599, 602, 594, 595, 593, 594, 592,
  595, 600, 600, 590, 590, 589, 596, 587,
  591, 595, 596, 592, 585, 589, 585, 582,
  581, 589, 589, 586, 582, 580, 576, 576,
  580, 581, 586, 584, 576, 576, 575, 576,
  573, 580, 583, 580, 576, 572, 581, 562
};
int mg_queen_table[64] = {
  682, 706, 706, 679, 718, 771, 784, 764,
  659, 657, 669, 678, 678, 723, 698, 719,
  665, 667, 674, 678, 697, 715, 706, 705,
  660, 664, 664, 665, 669, 674, 673, 674, 
  664, 658, 665, 663, 666, 667, 674, 669,
  664, 670, 661, 665, 664, 669, 680, 677,
  662, 662, 677, 671, 672, 681, 670, 672,
  661, 658, 661, 670, 660, 657, 660, 655
};
int eg_queen_table[64] = {
  1059, 1057, 1072, 1100, 1087, 1056, 1024, 1038,
  1054, 1082, 1096, 1097, 1109, 1090, 1087, 1051,
  1043, 1063, 1073, 1089, 1091, 1087, 1083, 1062,
  1044, 1059, 1076, 1095, 1106, 1096, 1090, 1071,
  1037, 1064, 1065, 1088, 1081, 1074, 1074, 1061,
  1020, 1037, 1061, 1056, 1060, 1058, 1047, 1028,
  1017, 1033, 1020, 1031, 1034, 1023, 1023, 1000,
  1015, 1020, 1026, 1015, 1026, 1009, 1005, 1002
};
int mg_king_table[64] = {
  17, 77, 6, -130, -28, -20, -38, 17,
  -6, -12, -27, -44, -41, -33, -70, -48,
  -24, -41, -32, -41, -40, -24, -28, -48,
  -30, -45, -40, -43, -63, -49, -36, -93,
  -56, -37, -36, -55, -57, -44, -41, -54,
  -10, -12, -21, -40, -38, -31, -11, -14,
  10, 10, -1, -42, -31, -9, 15, 14,
  -7, 35, 19, -44, 11, -13, 27, 18
};
int eg_king_table[64] = {
  -67, -52, -6, 33, 14, 16, 18, -20,
  -1, 19, 26, 29, 36, 38, 43, 16, 
  6, 30, 28, 30, 36, 45, 45, 21,
  -4, 28, 26, 31, 36, 37, 35, 23,
  -18, 9, 17, 24, 24, 20, 15, -6,
  -27, -8, 2, 10, 12, 7, -5, -16,
  -40, -22, -12, -3, -2, -10, -17, -28,
  -64, -45, -38, -25, -44, -30, -35, -55
};
int mg_passedPawnBonus[8] = {0, 2, 3, 6, 14, 8, 71, 0};
int eg_passedPawnBonus[8] = {0, 24, 16, 32, 59, 72, 187, 0};
// [238s] Epoch 2800 (13.6639 eps), error 0.00876295, LR 0.762343

int* passedPawnBonuses[2] = {mg_passedPawnBonus, eg_passedPawnBonus};

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

//A simple 768->N*2->1 NNUE
#define NNUEhiddenNeurons 128

int switchPieceColor[13] = {0, 7, 8, 9, 10, 11, 12, 1, 2, 3, 4, 5, 6};

struct NNUEparameters{
    alignas(32) std::array<std::array<int16_t, NNUEhiddenNeurons>, 768> hiddenLayerWeights;
    alignas(32) std::array<int16_t, NNUEhiddenNeurons> hiddenLayerBiases;
    alignas(32) std::array<int16_t, 2*NNUEhiddenNeurons> outputLayerWeights;
    int16_t outputLayerBias;
};

extern "C" {
  INCBIN(networkData, "vesta-5.nnue");
}
const NNUEparameters* _NNUEparameters = reinterpret_cast<const NNUEparameters *>(gnetworkDataData);

struct NNUE{
  std::array<std::array<int16_t, NNUEhiddenNeurons>, 2> accumulator = {{0}};

  int evaluate(chess::Colors sideToMove){
    int result = 0;

    bool currSide = sideToMove;
    for(int a=0; a<2; a++){
      for(int i=0; i<NNUEhiddenNeurons; i++){
        int v = std::max(std::min(int(accumulator[currSide][i]), 255), 0);
        v *= v;
        result += v * _NNUEparameters->outputLayerWeights[a*NNUEhiddenNeurons+i];
      }
      currSide = !currSide;
    }
    result /= 255;
    
    result = (result + _NNUEparameters->outputLayerBias) * 400 / (255*64);

    return result;
  }

  void refreshAccumulator(chess::Board& board){
    for(int i=0; i<NNUEhiddenNeurons; i++){
      accumulator[0][i] = _NNUEparameters->hiddenLayerBiases[i];
      accumulator[1][i] = _NNUEparameters->hiddenLayerBiases[i];
    }

    for(int square=0; square<64; square++){
      if(board.mailbox[0][square]!=0){
        int currFeatureIndex[2] = {64*(board.mailbox[0][square]-1)+square, 64*(switchPieceColor[board.mailbox[0][square]]-1)+(square^56)};
        for(int i=0; i<NNUEhiddenNeurons; i++){
          accumulator[0][i] += _NNUEparameters->hiddenLayerWeights[currFeatureIndex[0]][i];
          accumulator[1][i] += _NNUEparameters->hiddenLayerWeights[currFeatureIndex[1]][i];
        }
      }
    }
  }

  void updateSingleFeature(chess::Board& board, uint8_t square, chess::Pieces newPieceType, chess::Colors newPieceColor = chess::WHITE){
    uint8_t squareFromBlackPerspective = square^56;

    int newPiece = (newPieceColor == chess::WHITE) || (newPieceType == chess::null) ? newPieceType : newPieceType+6;

    int currFeatureIndex[2] = {64*(board.mailbox[0][square]-1)+square, 64*(board.mailbox[1][squareFromBlackPerspective]-1)+squareFromBlackPerspective};
    int newFeatureIndex[2] = {64*(newPiece-1)+square, 64*(switchPieceColor[newPiece]-1)+squareFromBlackPerspective};

    for(int i=0; i<NNUEhiddenNeurons; i++){
      if(board.mailbox[0][square] != 0){
        accumulator[0][i] -= _NNUEparameters->hiddenLayerWeights[currFeatureIndex[0]][i];
        accumulator[1][i] -= _NNUEparameters->hiddenLayerWeights[currFeatureIndex[1]][i];
      }
      if(newPieceType != chess::null){
        accumulator[0][i] += _NNUEparameters->hiddenLayerWeights[newFeatureIndex[0]][i];
        accumulator[1][i] += _NNUEparameters->hiddenLayerWeights[newFeatureIndex[1]][i];
      }
    }
  }

  void updateAccumulator(chess::Board& board, chess::Move move){
    U64 newHash;
    if(board.hashed){
      newHash = zobrist::updateHash(board, move);
    }

    board.halfmoveClock++;
    const uint8_t startSquare = move.getStartSquare();
    const uint8_t endSquare = move.getEndSquare();
    const chess::Pieces movingPiece = board.findPiece(startSquare);
    const chess::MoveFlags moveFlags = move.getMoveFlags();

    updateSingleFeature(board, startSquare, chess::null);
    board.mailbox[0][startSquare] = 0; board.mailbox[1][startSquare^56] = 0;
    board.unsetColors((1ULL << startSquare), board.sideToMove);
    board.unsetPieces(movingPiece, (1ULL << startSquare));

    if(moveFlags == chess::ENPASSANT){
      U64 theirPawnSquare;
      if(board.sideToMove == chess::WHITE){theirPawnSquare = (1ULL << endSquare) >> 8;}
      else{theirPawnSquare = (1ULL << endSquare) << 8;}

      uint8_t theirPawnSq = _bitscanForward(theirPawnSquare);

      updateSingleFeature(board, theirPawnSq, chess::null);
      board.mailbox[0][theirPawnSq] = 0; board.mailbox[1][theirPawnSq^56] = 0;
      board.unsetColors(theirPawnSquare, chess::Colors(!board.sideToMove));
      board.unsetPieces(chess::PAWN, theirPawnSquare);
    }
    else{
      if(board.getTheirPieces() & (1ULL << endSquare)){
        board.halfmoveClock = 0;
        board.startHistoryIndex = 0;

        updateSingleFeature(board, endSquare, chess::null);
        board.mailbox[0][endSquare] = 0; board.mailbox[1][endSquare^56] = 0;
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

      updateSingleFeature(board, rookStartSquare, chess::null);
      board.mailbox[0][rookStartSquare] = 0; board.mailbox[1][rookStartSquare^56] = 0;
      board.unsetColors(rookStartSquare, board.sideToMove);
      board.unsetPieces(chess::ROOK, rookStartSquare);

      updateSingleFeature(board, rookEndSquare, chess::ROOK, board.sideToMove);
      board.mailbox[0][rookEndSquare] = board.sideToMove ? 10 : 4; board.mailbox[1][rookEndSquare^56] = board.sideToMove ? 4 : 10;
      board.setColors(rookEndSquare, board.sideToMove);
      board.setPieces(chess::ROOK, rookEndSquare);
    }

    if(moveFlags == chess::PROMOTION){
      updateSingleFeature(board, endSquare, move.getPromotionPiece(), board.sideToMove);
      board.mailbox[0][endSquare] = board.sideToMove ? move.getPromotionPiece()+6 : move.getPromotionPiece();
      board.mailbox[1][endSquare^56] = board.sideToMove ? move.getPromotionPiece() : move.getPromotionPiece()+6;
      board.setPieces(move.getPromotionPiece(), (1ULL << endSquare));
    }
    else{
      updateSingleFeature(board, endSquare, movingPiece, board.sideToMove);
      board.mailbox[0][endSquare] = board.sideToMove ? movingPiece+6 : movingPiece;
      board.mailbox[1][endSquare^56] = board.sideToMove ? movingPiece : movingPiece+6;
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
  }
};

void init(){
  lookupTables::init();
}

int gamephaseInc[6] = {0, 1, 1, 2, 4, 0};

int gamePhase = 24;

int currentPieceSquareTableEval = 0;

int pieceSquareTable(chess::Board& board){
  gamePhase = 0;

  int mgScore = 0;
  int egScore = 0;

  /* evaluate each piece */
  for(int piece = chess::PAWN; piece <= chess::KING; piece++){
    U64 pieceBitboard = board.getOurPieces(chess::Pieces(piece));
    if(board.sideToMove == chess::WHITE){pieceBitboard = _flipBoard(pieceBitboard);}

    while(pieceBitboard){
      uint8_t piecePos = _popLsb(pieceBitboard);

      mgScore += mg_table[piece-1][piecePos];
      egScore += eg_table[piece-1][piecePos];

      gamePhase += gamephaseInc[piece-1];
    }

    pieceBitboard = board.getTheirPieces(chess::Pieces(piece));
    if(!board.sideToMove == chess::WHITE){pieceBitboard = _flipBoard(pieceBitboard);}

    while(pieceBitboard){
      uint8_t piecePos = _popLsb(pieceBitboard);

      mgScore -= mg_table[piece-1][piecePos];
      egScore -= eg_table[piece-1][piecePos];

      gamePhase += gamephaseInc[piece-1];
    }
  }

  /* tapered eval */
  if (gamePhase > 24){gamePhase = 24;} /* in case of early promotion */

  currentPieceSquareTableEval = (mgScore * gamePhase + egScore * (24-gamePhase))/24;
  return currentPieceSquareTableEval;
}

//Static Exchange Evaluation
//Returns the value in cp from the current board's sideToMove's perspective on how good capturing an enemy piece on targetSquare is
//Returns 0 if the capture is not good for the current board's sideToMove or if there is no capture
//Threshold is the highest SEE value we have already found (see the part in evaluate() which runs SEE())
int SEE(chess::Board& board, uint8_t targetSquare, int threshold = 0){
  int values[32];
  int i=0;

  chess::Pieces currPiece = board.findPiece(targetSquare); //The original target piece; piece of the opponent of the current sideToMove
  values[i] = (mg_value[currPiece-1] * gamePhase + eg_value[currPiece-1] * (24-gamePhase));
  /*values[i] = (
    (mg_value[currPiece-1] + mg_table[currPiece-1][board.sideToMove ? targetSquare^56 : targetSquare]) * gamePhase
   +(eg_value[currPiece-1] + eg_table[currPiece-1][board.sideToMove ? targetSquare^56 : targetSquare]) * (24-gamePhase));*/

  chess::Colors us = board.sideToMove;
  U64 white = board.white;
  U64 black = board.black;

  board.sideToMove = chess::Colors(!board.sideToMove);
  bool isOurSideToMove = false;

  uint8_t piecePos = board.squareUnderAttack(targetSquare);

  //See https://www.chessprogramming.org/Alpha-Beta#Negamax_Framework for the recursive implementation this implementation is based on
  int alpha = -999999;
  int beta = -(threshold*24);

  while(piecePos<=63){
    i++;

    //Alpha Beta pruning does not affect the result of the SEE
    if(-values[i-1] >= beta){break;}
    if(-values[i-1] > alpha){alpha = -values[i-1];}
    int placeholder = alpha; alpha = -beta; beta = -placeholder;

    chess::Pieces leastValuableAttacker = board.findPiece(piecePos);
    //The value for the enemy of the side of the leastValuableAttacker if the leastValuableAttacker is captured
    values[i] = (mg_value[leastValuableAttacker-1] * gamePhase + eg_value[leastValuableAttacker-1] * (24-gamePhase)) - values[i-1];
    /*uint8_t _piecePos = board.sideToMove ? piecePos^56 : piecePos; //for use in pieceSquareTable calculation below
    values[i-1] += (-mg_table[leastValuableAttacker-1][_piecePos] + mg_table[leastValuableAttacker-1][board.sideToMove ? targetSquare^56 : targetSquare]) * gamePhase
                  +(-eg_table[leastValuableAttacker-1][_piecePos] + eg_table[leastValuableAttacker-1][board.sideToMove ? targetSquare^56 : targetSquare]) * (24-gamePhase);
    values[i] = (mg_value[leastValuableAttacker-1] * gamePhase + eg_value[leastValuableAttacker-1] * (24-gamePhase)) - (values[i-1] - mg_table[leastValuableAttacker-1][board.sideToMove ? targetSquare^56 : targetSquare] * gamePhase - eg_table[leastValuableAttacker-1][board.sideToMove ? targetSquare^56 : targetSquare] * (24-gamePhase));*/


    board.sideToMove = chess::Colors(!board.sideToMove); isOurSideToMove = !isOurSideToMove;
    board.unsetColors(1ULL << piecePos, chess::Colors(isOurSideToMove ? us : !us));

    piecePos = board.squareUnderAttack(targetSquare);
  }

  board.sideToMove = us;
  board.white = white;
  board.black = black;

  return (isOurSideToMove ? beta : -beta)/24;
}

int passedPawns(chess::Board& board){
  int mg_score = 0;
  int eg_score = 0;
  U64 pieceBitboard = board.getOurPieces(chess::PAWN);
  U64 theirPawns = board.getTheirPieces(chess::PAWN);
  while(pieceBitboard){
    uint8_t piecePos = _popLsb(pieceBitboard);
    if((lookupTables::passedPawnTable[board.sideToMove][piecePos] & theirPawns) == 0ULL){
      uint8_t pawnRank = squareIndexToRank(piecePos);
      if(board.sideToMove == chess::WHITE){
        mg_score += passedPawnBonuses[0][pawnRank];
        eg_score += passedPawnBonuses[1][pawnRank];
      }
      else{
        mg_score += passedPawnBonuses[0][7-pawnRank];
        eg_score += passedPawnBonuses[1][7-pawnRank];
      }
    }
  }
  pieceBitboard = board.getTheirPieces(chess::PAWN);
  theirPawns = board.getOurPieces(chess::PAWN);
  while(pieceBitboard){
    uint8_t piecePos = _popLsb(pieceBitboard);
    if((lookupTables::passedPawnTable[!board.sideToMove][piecePos] & theirPawns) == 0ULL){
      uint8_t pawnRank = squareIndexToRank(piecePos);
      if((!board.sideToMove) == chess::WHITE){
        mg_score -= passedPawnBonuses[0][pawnRank];
        eg_score -= passedPawnBonuses[1][pawnRank];
      }
      else{
        mg_score -= passedPawnBonuses[0][7-pawnRank];
        eg_score -= passedPawnBonuses[1][7-pawnRank];
      }
    }
  }
  return (gamePhase*mg_score+(24-gamePhase)*eg_score)/24;
}

std::array<uint8_t, 13> sidedPieceToPiece = {0, 1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6};

int mvvLva(chess::Board& board, chess::Move move){
  return 30*mg_value[sidedPieceToPiece[board.mailbox[0][move.getEndSquare()]]-1] - mg_value[sidedPieceToPiece[board.mailbox[0][move.getStartSquare()]]-1];
}

int qSearch(chess::Board& board, NNUE& nnue, int alpha, int beta){
  int eval = nnue.evaluate(board.sideToMove);
  int bestEval = eval;

  if(eval >= beta){return eval;}

  if(eval > alpha){alpha = eval;}

  chess::MoveList moves(board, true);

  std::array<int, 256> orderValue;
  int i=0;
  for(auto move : moves){orderValue[i] = mvvLva(board, move); i++;}

  std::array<std::array<int16_t, NNUEhiddenNeurons>, 2> currAccumulator = nnue.accumulator;

  for(uint32_t i=0; i<moves.size(); i++){
    //std::cout << "\n";
    for(uint32_t j=i+1; j<moves.size(); j++) {
      if(orderValue[j] > orderValue[i]) {
          //std::cout << orderValue[j] << ":" << int(sidedPieceToPiece[board.mailbox[0][moves[j].getEndSquare()]]) << ":" << int(sidedPieceToPiece[board.mailbox[0][moves[j].getStartSquare()]]) << " ";
          std::swap(orderValue[j], orderValue[i]);
          std::swap(moves.moveList[j], moves.moveList[i]);
      }
    }
    if(SEE(board, moves[i].getEndSquare(), 0) == 0) continue;

    chess::Board movedBoard = board;
    nnue.accumulator = currAccumulator;
    nnue.updateAccumulator(movedBoard, moves[i]);

    eval = -qSearch(movedBoard, nnue, -beta, -alpha);
    
    if(eval > bestEval) bestEval = eval;
    if(eval > alpha) alpha = eval;
    if(eval >= beta) break;
  }

  return bestEval;
}

int evaluate(chess::Board& board, NNUE& nnue){
  int cpEvaluation = qSearch(board, nnue, -999999, 999999);

  return cpEvaluation;
}

//Takes in the current board and a list of moves from the current position
//Then calculates the static evaluation of all children of the node of the current board
//It incrementally changes the evaluation based on the move, so it is much faster than running evaluate() on each child position
//Returns a pointer to the start of the array with all of the evaluations
/*int* evaluateAllChildren(chess::Board& board, chess::MoveList moves){
  const int parentEval = evaluate(board);

  int results[moves.size()];
  int* currResult = results;

  for(chess::Move move : moves){
    *currResult = parentEval;
    const uint8_t startSquare = move.getStartSquare();
    const uint8_t endSquare = move.getEndSquare();
    const chess::Pieces movingPiece = board.findPiece(startSquare);
    const chess::MoveFlags moveFlags = move.getMoveFlags();

    *currResult -= (mg_value[movingPiece-1] + mg_table[movingPiece-1][startSquare]);

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
  return 0;
}*/
}