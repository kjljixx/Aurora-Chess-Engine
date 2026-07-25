#include "framework.h"
#include "test_helpers.h"
#include "zobrist.h"

TEST(zobrist, incremental_matches_recompute_after_quiet_move) {
  chess::Board board;
  chess::makeMove(board, aurora_test::findMove(board, "e2e4"));

  ASSERT_EQ(board.history[board.halfmoveClock], zobrist::getHash(board));
}

TEST(zobrist, incremental_matches_recompute_after_capture) {
  chess::Board board("rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2");
  chess::makeMove(board, aurora_test::findMove(board, "e4d5"));

  ASSERT_EQ(board.history[board.halfmoveClock], zobrist::getHash(board));
}

TEST(zobrist, incremental_matches_recompute_after_castle) {
  chess::Board board("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
  chess::makeMove(board, aurora_test::findMove(board, "e1g1"));

  ASSERT_EQ(board.history[board.halfmoveClock], zobrist::getHash(board));
}

TEST(zobrist, incremental_matches_recompute_after_ep) {
  chess::Board board("8/8/8/2k5/3pP3/8/8/4K3 b - e3 0 1");
  chess::makeMove(board, aurora_test::findMove(board, "d4e3"));

  ASSERT_EQ(board.history[board.halfmoveClock], zobrist::getHash(board));
}

TEST(zobrist, incremental_matches_recompute_after_promotion) {
  chess::Board board("8/4P3/8/8/8/8/8/4k2K w - - 0 1");
  chess::Move promo = aurora_test::findMove(board, "e7e8q");
  ASSERT_EQ(promo.getMoveFlags(), chess::PROMOTION);
  chess::makeMove(board, promo);

  ASSERT_EQ(board.history[board.halfmoveClock], zobrist::getHash(board));
}

TEST(zobrist, multi_castling_rights_change_on_corner_capture) {
  // Ra1xa8 clears white queenside (mover) and black queenside (captured rook)
  chess::Board board("r3k3/8/8/8/8/8/8/R3K3 w Qq - 0 1");
  ASSERT_EQ(board.castlingRights, 0xAu); // Q|q

  chess::makeMove(board, aurora_test::findMove(board, "a1a8"));

  ASSERT_EQ(board.castlingRights, 0u);
  ASSERT_EQ(board.history[board.halfmoveClock], zobrist::getHash(board));
}

TEST(zobrist, ep_only_hashed_when_capturable) {
  // After e2e4 from the start position, no black pawn attacks e3 yet
  chess::Board notCapturable("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
  ASSERT_FALSE(zobrist::shouldHashEnPassant(notCapturable));

  // Black pawn on d4 can capture EP onto e3
  chess::Board capturable("4k3/8/8/8/3pP3/8/8/4K3 b - e3 0 1");
  ASSERT_TRUE(zobrist::shouldHashEnPassant(capturable));
}

TEST(zobrist, sequence_of_moves_stays_consistent) {
  chess::Board board;
  const char* moves[] = {"e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "g8f6"};
  for (const char* uci : moves) {
    chess::Move move = aurora_test::findMove(board, uci);
    ASSERT_TRUE(move.value != 0);
    chess::makeMove(board, move);
    ASSERT_EQ(board.history[board.halfmoveClock], zobrist::getHash(board));
  }
}
