#pragma once

#include "chess.h"
#include "zobrist.h"

#include <string>

namespace aurora_test {

inline uint64_t perft(chess::Board& board, int depth) {
  if (depth == 0) {
    return 1;
  }

  if (depth == 1) {
    return chess::MoveList(board).size();
  }

  uint64_t nodes = 0;
  for (const chess::Move move : chess::MoveList(board)) {
    chess::Board child = board;
    child.makeMove(move);
    nodes += perft(child, depth - 1);
  }
  return nodes;
}

inline bool moveListContains(chess::Board& board, const std::string& uci) {
  for (chess::Move move : chess::MoveList(board)) {
    if (move.toStringRep() == uci) {
      return true;
    }
  }
  return false;
}

inline chess::Move findMove(chess::Board& board, const std::string& uci) {
  for (chess::Move move : chess::MoveList(board)) {
    if (move.toStringRep() == uci) {
      return move;
    }
  }
  return chess::Move();
}

inline int countMovesWithFlag(chess::Board& board, chess::MoveFlags flag) {
  int count = 0;
  for (chess::Move move : chess::MoveList(board)) {
    if (move.getMoveFlags() == flag) {
      count++;
    }
  }
  return count;
}

inline bool boardsEqualFull(const chess::Board& a, const chess::Board& b) {
  return a.pawns == b.pawns
      && a.knights == b.knights
      && a.bishops == b.bishops
      && a.rooks == b.rooks
      && a.queens == b.queens
      && a.kings == b.kings
      && a.white == b.white
      && a.black == b.black
      && a.occupied == b.occupied
      && a.sideToMove == b.sideToMove
      && a.castlingRights == b.castlingRights
      && a.enPassant == b.enPassant
      && a.halfmoveClock == b.halfmoveClock
      && a.mailbox == b.mailbox;
}

} // namespace aurora_test
