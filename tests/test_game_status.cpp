#include "framework.h"
#include "test_helpers.h"
#include "zobrist.h"

TEST(game_status, checkmate) {
  chess::Board board("4k3/4Q3/4K3/8/8/8/8/8 b - - 0 1");
  ASSERT_FALSE(chess::isLegalMoves(board));
  ASSERT_TRUE(board.squareUnderAttack(bitscanForward(board.getOurPieces(chess::KING))) <= 63);
  ASSERT_EQ(static_cast<int>(chess::getGameStatus(board, false)), static_cast<int>(chess::LOSS));
}

TEST(game_status, stalemate) {
  // Black king h8: Kg6 + Pf7 cover all escapes; pawn does not check h8
  chess::Board board("7k/5P2/6K1/8/8/8/8/8 b - - 0 1");
  ASSERT_FALSE(chess::isLegalMoves(board));
  ASSERT_FALSE(board.squareUnderAttack(bitscanForward(board.getOurPieces(chess::KING))) <= 63);
  ASSERT_EQ(static_cast<int>(chess::getGameStatus(board, false)), static_cast<int>(chess::DRAW));
}

TEST(game_status, insufficient_material_king_vs_king) {
  chess::Board board("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
  ASSERT_TRUE(chess::isLegalMoves(board));
  ASSERT_EQ(chess::getGameStatus(board, true), chess::DRAW);
}

TEST(game_status, insufficient_material_king_and_bishop) {
  chess::Board board("4k3/8/8/8/8/8/8/4KB2 w - - 0 1");
  ASSERT_EQ(chess::getGameStatus(board, true), chess::DRAW);
}

TEST(game_status, insufficient_material_king_and_knight) {
  chess::Board board("4k3/8/8/8/8/8/8/4KN2 w - - 0 1");
  ASSERT_EQ(chess::getGameStatus(board, true), chess::DRAW);
}

TEST(game_status, not_insufficient_with_pawn) {
  chess::Board board("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1");
  ASSERT_EQ(chess::getGameStatus(board, true), chess::ONGOING);
}

TEST(game_status, fifty_move_rule) {
  chess::Board board("4k3/8/8/8/8/8/4R3/4K3 w - - 100 1");
  ASSERT_EQ(board.halfmoveClock, 100);
  ASSERT_EQ(chess::getGameStatus(board, true), chess::DRAW);
}

TEST(game_status, threefold_repetition) {
  chess::Board board("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
  // Manually seed a threefold: same hash at indices 0, 2, and current 4
  const U64 hash = zobrist::getHash(board);
  board.hashed = true;
  board.startHistoryIndex = 0;
  board.halfmoveClock = 4;
  board.history[0] = hash;
  board.history[1] = hash ^ 0x1234;
  board.history[2] = hash;
  board.history[3] = hash ^ 0x5678;
  board.history[4] = hash;

  ASSERT_EQ(chess::getGameStatus(board, true), chess::DRAW);
}

TEST(game_status, twofold_is_not_draw) {
  // Include a pawn so insufficient-material draw does not trigger first
  chess::Board board("4k3/8/8/8/8/4P3/8/4K3 w - - 0 1");
  const U64 hash = zobrist::getHash(board);
  board.hashed = true;
  board.startHistoryIndex = 0;
  board.halfmoveClock = 2;
  board.history[0] = hash;
  board.history[1] = hash ^ 0x1234ULL;
  board.history[2] = hash;

  ASSERT_EQ(static_cast<int>(chess::getGameStatus(board, true)), static_cast<int>(chess::ONGOING));
}

TEST(game_status, is_legal_moves_agrees_with_move_list) {
  const char* fens[] = {
    chess::startPosFen.c_str(),
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
    "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1",
    "7k/5P2/6K1/8/8/8/8/8 b - - 0 1",
    "8/8/8/8/k2pP2R/8/8/4K3 b - e3 0 1",
  };

  for (const char* fen : fens) {
    chess::Board board(fen);
    const bool hasMoves = chess::MoveList(board).size() > 0;
    ASSERT_EQ(chess::isLegalMoves(board), hasMoves);
  }
}
