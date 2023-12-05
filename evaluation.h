#pragma once
#include "zobrist.h"
#include <fstream>
#include <math.h>
#include <algorithm>
#include <random>
#include <filesystem>

namespace evaluation{

const int EVAL_VERSION = 1;

const int evalSharpeningFactor = 100000; //Generally, the higher this parameter is, the more accurate the eval (Unless integer overflow occurs)

//Included EvalFile trained on 500k fens using Texel Tuner
//[7840s] Epoch 5900 (0.754214 eps), error 0.00398793, LR 0.0111653
std::string evalFile = "eval.auroraeval";

int piecePairTable[64][13][64][13];

int switchPieceColor[13] = {0, 7, 8, 9, 10, 11, 12, 1, 2, 3, 4, 5, 6};

void init(){
  std::ifstream pptFile{evalFile, std::ios::binary};
  double token;

  if(!std::filesystem::exists(evalFile)){
    std::cout << "info string failed to load evalfile; could not find evalfile\n";
    return;
  }
  
  if(!pptFile.good()){
    std::cout << "info string failed to load evalfile; could not open evalfile\n";
    return;
  }
  pptFile >> token;
  if(token != EVAL_VERSION){
    std::cout << "info string failed to load evalfile; the version of the evalfile is not compatible with the engine\n";
    return;
  }

  int iter = 0;
  for(int i=0; i<64; i++){
    for(int j=0; j<13; j++){
      for(int k=i+1; k<64; k++){
        for(int l=0; l<13; l++){
          pptFile >> token;
          piecePairTable[i][j][k][l] = int(token*100000);
          piecePairTable[k][l][i][j] = int(token*100000);

          iter++;
        }
      }
    }
  }

  std::cout << "info string evalfile loaded successfully\n";
}

struct Eval{
  int whiteToMove;
  int blackToMove;

  Eval() : whiteToMove(0), blackToMove(0) {}

  Eval(int white, int black) : whiteToMove(white), blackToMove(black) {}
};

int gamePhase = 24;
int gamephaseInc[6] = {0, 1, 1, 2, 4, 0};

int calcGamePhase(chess::Board& board){
  gamePhase = 0;

  /* evaluate each piece */
  for(int piece = chess::PAWN; piece < chess::KING; piece++){
    U64 pieceBitboard = board.getPieces(chess::Pieces(piece));
    gamePhase += gamephaseInc[piece-1]*_popCount(pieceBitboard);
  }

  if (gamePhase > 24){gamePhase = 24;} /* in case of early promotion */
  return gamePhase;
}

int mg_value[6] = {42, 184, 207, 261, 642, 10000};
int eg_value[6] = {71, 242, 265, 538, 1067, 10000};
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

int mg_passedPawnBonus[8] = {0, 2, 3, 6, 14, 8, 71, 0};
int eg_passedPawnBonus[8] = {0, 24, 16, 32, 59, 72, 187, 0};
int* passedPawnBonuses[2] = {mg_passedPawnBonus, eg_passedPawnBonus};

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

Eval updateEvalOnSquare(Eval prevEval, chess::Board& board, uint8_t square, chess::Pieces newPieceType, chess::Colors newPieceColor = chess::WHITE){
  int changingPiece = board.mailbox[0][square];

  int newPiece = (newPieceColor == chess::WHITE) || (newPieceType == chess::null) ? newPieceType : newPieceType+6;
  
  int currPiece;

  for(int i=0; i<square; i++){
    currPiece = board.mailbox[0][i];
    
    prevEval.whiteToMove -= piecePairTable[square][changingPiece][i][currPiece];
    prevEval.whiteToMove += piecePairTable[square][newPiece][i][currPiece];
  }
  for(int i=square+1; i<64; i++){
    currPiece = board.mailbox[0][i];
    
    prevEval.whiteToMove -= piecePairTable[square][changingPiece][i][currPiece];
    prevEval.whiteToMove += piecePairTable[square][newPiece][i][currPiece];
  }

  changingPiece = switchPieceColor[changingPiece];

  newPiece = switchPieceColor[newPiece];

  square ^= 56;

  for(int i=0; i<square; i++){
    currPiece = board.mailbox[1][i];
    
    prevEval.blackToMove -= piecePairTable[square][changingPiece][i][currPiece];
    prevEval.blackToMove += piecePairTable[square][newPiece][i][currPiece];
  }
  for(int i=square+1; i<64; i++){
    currPiece = board.mailbox[1][i];
    
    prevEval.blackToMove -= piecePairTable[square][changingPiece][i][currPiece];
    prevEval.blackToMove += piecePairTable[square][newPiece][i][currPiece];
  }

  return prevEval;
}

//Incrementally updates the Eval given based on a move made on the board
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
  board.mailbox[0][startSquare] = 0; board.mailbox[1][startSquare^56] = 0;
  board.unsetColors((1ULL << startSquare), board.sideToMove);
  board.unsetPieces(movingPiece, (1ULL << startSquare));

  if(moveFlags == chess::ENPASSANT){
    U64 theirPawnSquare;
    if(board.sideToMove == chess::WHITE){theirPawnSquare = (1ULL << endSquare) >> 8;}
    else{theirPawnSquare = (1ULL << endSquare) << 8;}

    uint8_t theirPawnSq = _bitscanForward(theirPawnSquare);

    prevEval = updateEvalOnSquare(prevEval, board, theirPawnSq, chess::null);
    board.mailbox[0][theirPawnSq] = 0; board.mailbox[1][theirPawnSq^56] = 0;
    board.unsetColors(theirPawnSquare, chess::Colors(!board.sideToMove));
    board.unsetPieces(chess::PAWN, theirPawnSquare);
  }
  else{
    if(board.getTheirPieces() & (1ULL << endSquare)){
      board.halfmoveClock = 0;
      board.startHistoryIndex = 0;

      prevEval = updateEvalOnSquare(prevEval, board, endSquare, chess::null);
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

    prevEval = updateEvalOnSquare(prevEval, board, rookStartSquare, chess::null);
    board.mailbox[0][rookStartSquare] = 0; board.mailbox[1][rookStartSquare^56] = 0;
    board.unsetColors(rookStartSquare, board.sideToMove);
    board.unsetPieces(chess::ROOK, rookStartSquare);

    prevEval = updateEvalOnSquare(prevEval, board, rookEndSquare, chess::ROOK, board.sideToMove);
    board.mailbox[0][rookEndSquare] = board.sideToMove ? 10 : 4; board.mailbox[1][rookEndSquare^56] = board.sideToMove ? 4 : 10;
    board.setColors(rookEndSquare, board.sideToMove);
    board.setPieces(chess::ROOK, rookEndSquare);
  }

  if(moveFlags == chess::PROMOTION){
    prevEval = updateEvalOnSquare(prevEval, board, endSquare, move.getPromotionPiece(), board.sideToMove);
    board.mailbox[0][endSquare] = board.sideToMove ? move.getPromotionPiece()+6 : move.getPromotionPiece();
    board.mailbox[1][endSquare^56] = board.sideToMove ? move.getPromotionPiece() : move.getPromotionPiece()+6;
    board.setPieces(move.getPromotionPiece(), (1ULL << endSquare));
  }
  else{
    prevEval = updateEvalOnSquare(prevEval, board, endSquare, movingPiece, board.sideToMove);
    board.mailbox[0][endSquare] = board.sideToMove ? movingPiece+6 : movingPiece;
    board.mailbox[1][endSquare] = board.sideToMove ? movingPiece : movingPiece+6;
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