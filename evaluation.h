#include "chess.h"
#include <math.h>

namespace evaluation{
  float evaluate(chess::Board& board){
    int32_t cpEvaluation = 0;

    cpEvaluation += _popCount(board.getOurPieces(chess::PAWN))*100;
    cpEvaluation += _popCount(board.getOurPieces(chess::KNIGHT))*300;
    cpEvaluation += _popCount(board.getOurPieces(chess::BISHOP))*310;
    cpEvaluation += _popCount(board.getOurPieces(chess::ROOK))*500;
    cpEvaluation += _popCount(board.getOurPieces(chess::QUEEN))*900;
    cpEvaluation -= _popCount(board.getTheirPieces(chess::PAWN))*100;
    cpEvaluation -= _popCount(board.getTheirPieces(chess::KNIGHT))*300;
    cpEvaluation -= _popCount(board.getTheirPieces(chess::BISHOP))*310;
    cpEvaluation -= _popCount(board.getTheirPieces(chess::ROOK))*500;
    cpEvaluation -= _popCount(board.getTheirPieces(chess::QUEEN))*900;

    std::cout << 2/(1+exp(-cpEvaluation*0.005*1.946))-1;
    return 2/(1+exp(-cpEvaluation*0.005*1.946))-1;
  }
}