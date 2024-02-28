#pragma once
#include "simd.h"
#include "zobrist.h"
#include <math.h>
#include <algorithm>
#include <random>
#include <array>
#include "external/incbin.h"

namespace evaluation{

int mg_value[6] = {42, 184, 207, 261, 642, 10000};

//A simple 768->N*2->1 NNUE
#define NNUEhiddenNeurons 128

const int WeightsPerVec = sizeof(SIMD::Vec) / sizeof(int16_t);

int switchPieceColor[13] = {0, 7, 8, 9, 10, 11, 12, 1, 2, 3, 4, 5, 6};

struct NNUEparameters{
    alignas(32) std::array<std::array<int16_t, NNUEhiddenNeurons>, 768> hiddenLayerWeights;
    alignas(32) std::array<int16_t, NNUEhiddenNeurons> hiddenLayerBiases;
    alignas(32) std::array<int16_t, 2*NNUEhiddenNeurons> outputLayerWeights;
    int16_t outputLayerBias;
};

extern "C" {
  INCBIN(networkData, "vesta-7.nnue");
}
const NNUEparameters* _NNUEparameters = reinterpret_cast<const NNUEparameters *>(gnetworkDataData);

struct NNUE{
  std::array<std::array<int16_t, NNUEhiddenNeurons>, 2> accumulator = {{0}};

  int evaluate(chess::Colors sideToMove){
    //Adapted from Obsidian https://github.com/gab8192/Obsidian/blob/main/Obsidian/nnue.cpp
    SIMD::Vec stmAcc;
    SIMD::Vec oppAcc;

    const SIMD::Vec vecZero = SIMD::vecSetZero();
    const SIMD::Vec vecQA = SIMD::vecSet1Epi16(255);

    SIMD::Vec sum = SIMD::vecSetZero();
    SIMD::Vec v0; SIMD::Vec v1;

    for (int i = 0; i < NNUEhiddenNeurons / WeightsPerVec; ++i) {
      // Side to move
      stmAcc = SIMD::load(reinterpret_cast<const SIMD::Vec *>(&accumulator[sideToMove][i * WeightsPerVec]));
      v0 = SIMD::maxEpi16(stmAcc, vecZero); // clip
      v0 = SIMD::minEpi16(v0, vecQA); // clip
      v1 = SIMD::mulloEpi16(v0, SIMD::load(reinterpret_cast<const SIMD::Vec *>(&_NNUEparameters->outputLayerWeights[i * WeightsPerVec]))); // multiply with output layer weights
      v1 = SIMD::maddEpi16(v0, v1); // square
      sum = SIMD::addEpi32(sum, v1); // collect the result

      // Non side to move
      oppAcc = SIMD::load(reinterpret_cast<const SIMD::Vec *>(&accumulator[!sideToMove][i * WeightsPerVec]));
      v0 = SIMD::maxEpi16(oppAcc, vecZero);
      v0 = SIMD::minEpi16(v0, vecQA);
      v1 = SIMD::mulloEpi16(v0, SIMD::load(reinterpret_cast<const SIMD::Vec *>(&_NNUEparameters->outputLayerWeights[NNUEhiddenNeurons + i * WeightsPerVec])));
      v1 = SIMD::maddEpi16(v0, v1);
      sum = SIMD::addEpi32(sum, v1);
    }
    int unsquared = SIMD::vecHaddEpi32(sum) / 255 + _NNUEparameters->outputLayerBias;

    return (unsquared * 400) / (255 * 64);
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

    if(board.mailbox[0][square] != 0){
      for(int i=0; i<NNUEhiddenNeurons; i++){
        accumulator[0][i] -= _NNUEparameters->hiddenLayerWeights[currFeatureIndex[0]][i];
        accumulator[1][i] -= _NNUEparameters->hiddenLayerWeights[currFeatureIndex[1]][i];
      }
    }
    if(newPieceType != chess::null){
      for(int i=0; i<NNUEhiddenNeurons; i++){
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

//Static Exchange Evaluation
//Returns the value in cp from the current board's sideToMove's perspective on how good capturing an enemy piece on targetSquare is
//Returns 0 if the capture is not good for the current board's sideToMove or if there is no capture
//Threshold is the highest SEE value we have already found (see the part in evaluate() which runs SEE())
int SEE(chess::Board& board, uint8_t targetSquare, int threshold = 0){
  int values[32];
  int i=0;

  chess::Pieces currPiece = board.findPiece(targetSquare); //The original target piece; piece of the opponent of the current sideToMove
  values[i] = mg_value[currPiece-1];

  chess::Colors us = board.sideToMove;
  U64 white = board.white;
  U64 black = board.black;

  board.sideToMove = chess::Colors(!board.sideToMove);
  bool isOurSideToMove = false;

  uint8_t piecePos = board.squareUnderAttack(targetSquare);

  //See https://www.chessprogramming.org/Alpha-Beta#Negamax_Framework for the recursive implementation this implementation is based on
  int alpha = -999999;
  int beta = -threshold;

  while(piecePos<=63){
    i++;

    //Alpha Beta pruning does not affect the result of the SEE
    if(-values[i-1] >= beta){break;}
    if(-values[i-1] > alpha){alpha = -values[i-1];}
    int placeholder = alpha; alpha = -beta; beta = -placeholder;

    chess::Pieces leastValuableAttacker = board.findPiece(piecePos);
    
    //The value for the enemy of the side of the leastValuableAttacker if the leastValuableAttacker is captured
    values[i] = mg_value[leastValuableAttacker-1] - values[i-1];

    board.sideToMove = chess::Colors(!board.sideToMove); isOurSideToMove = !isOurSideToMove;
    board.unsetColors(1ULL << piecePos, chess::Colors(isOurSideToMove ? us : !us));

    piecePos = board.squareUnderAttack(targetSquare);
  }

  board.sideToMove = us;
  board.white = white;
  board.black = black;

  return (isOurSideToMove ? beta : -beta);
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
} //namespace evaluation
