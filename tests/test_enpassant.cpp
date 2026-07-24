#include "framework.h"
#include "test_helpers.h"

TEST(enpassant, legal_capture_is_generated) {
  chess::Board board("8/8/8/2k5/3pP3/8/8/4K3 b - e3 0 1");

  ASSERT_TRUE(aurora_test::moveListContains(board, "d4e3"));
  chess::Move ep = aurora_test::findMove(board, "d4e3");
  ASSERT_EQ(ep.getMoveFlags(), chess::ENPASSANT);
}

TEST(enpassant, illegal_when_it_discovers_horizontal_check) {
  // Capturing EP would open Ka4 to Rh4
  chess::Board board("8/8/8/8/k2pP2R/8/8/4K3 b - e3 0 1");

  ASSERT_FALSE(aurora_test::moveListContains(board, "d4e3"));
}

TEST(enpassant, legal_when_rook_not_on_rank) {
  chess::Board board("8/8/8/8/k2pP3/8/8/4K2R b - e3 0 1");

  ASSERT_TRUE(aurora_test::moveListContains(board, "d4e3"));
}

TEST(enpassant, make_move_removes_captured_pawn) {
  chess::Board board("8/8/8/2k5/3pP3/8/8/4K3 b - e3 0 1");
  chess::Move ep = aurora_test::findMove(board, "d4e3");
  board.makeMove(ep);

  ASSERT_EQ(board.findPiece(squareNotationToIndex("e4")), chess::null);
  ASSERT_EQ(board.findPiece(squareNotationToIndex("e3")), chess::PAWN);
  ASSERT_EQ(board.findPiece(squareNotationToIndex("d4")), chess::null);
  ASSERT_TRUE(board.black & (1ULL << squareNotationToIndex("e3")));
  ASSERT_FALSE(board.white & (1ULL << squareNotationToIndex("e4")));
  ASSERT_EQ(board.enPassant, 0ULL);
}

TEST(enpassant, double_push_sets_ep_square) {
  chess::Board board;
  chess::Move push = aurora_test::findMove(board, "e2e4");
  board.makeMove(push);

  ASSERT_EQ(board.enPassant, 1ULL << squareNotationToIndex("e3"));
}

TEST(enpassant, ep_square_cleared_on_quiet_move) {
  chess::Board board("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
  chess::Move move = aurora_test::findMove(board, "e7e5");
  board.makeMove(move);

  // Black double-pushed, so new EP should be e6 — not the old e3
  ASSERT_EQ(board.enPassant, 1ULL << squareNotationToIndex("e6"));
}

TEST(enpassant, white_ep_capture) {
  chess::Board board("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");

  ASSERT_TRUE(aurora_test::moveListContains(board, "e5d6"));
  chess::Move ep = aurora_test::findMove(board, "e5d6");
  ASSERT_EQ(ep.getMoveFlags(), chess::ENPASSANT);
  board.makeMove(ep);

  ASSERT_EQ(board.findPiece(squareNotationToIndex("d5")), chess::null);
  ASSERT_EQ(board.findPiece(squareNotationToIndex("d6")), chess::PAWN);
}

TEST(enpassant, capture_generator_includes_ep) {
  chess::Board board("8/8/8/2k5/3pP3/8/8/4K3 b - e3 0 1");
  chess::MoveList captures(board, true);

  bool found = false;
  for (chess::Move move : captures) {
    if (move.toStringRep() == "d4e3" && move.getMoveFlags() == chess::ENPASSANT) {
      found = true;
    }
  }
  ASSERT_TRUE(found);
}

TEST(enpassant, pinned_pawn_cannot_ep_off_pin_ray) {
  // Black king e8, black pawn e4, white rook e1 — vertical pin; EP d3 leaves the ray
  chess::Board pinned("4k3/8/8/8/3Pp3/8/8/4R3 b - d3 0 1");

  ASSERT_FALSE(aurora_test::moveListContains(pinned, "e4d3"));
}
