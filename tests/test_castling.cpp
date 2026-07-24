#include "framework.h"
#include "test_helpers.h"

TEST(castling, start_like_position_allows_both_sides) {
  chess::Board board("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");

  ASSERT_TRUE(aurora_test::moveListContains(board, "e1g1"));
  ASSERT_TRUE(aurora_test::moveListContains(board, "e1c1"));
  ASSERT_EQ(aurora_test::countMovesWithFlag(board, chess::CASTLE), 2);
}

TEST(castling, blocked_by_piece_on_f1) {
  chess::Board board("r3k2r/8/8/8/8/8/8/R3KB1R w KQkq - 0 1");

  ASSERT_FALSE(aurora_test::moveListContains(board, "e1g1"));
  ASSERT_TRUE(aurora_test::moveListContains(board, "e1c1"));
}

TEST(castling, blocked_by_attack_on_f1) {
  chess::Board board("r3k2r/8/8/8/8/5q2/8/R3K2R w KQkq - 0 1");

  ASSERT_FALSE(aurora_test::moveListContains(board, "e1g1"));
}

TEST(castling, blocked_by_attack_on_g1) {
  chess::Board board("r3k2r/8/8/8/8/6q1/8/R3K2R w KQkq - 0 1");

  ASSERT_FALSE(aurora_test::moveListContains(board, "e1g1"));
}

TEST(castling, not_allowed_while_in_check) {
  chess::Board board("r3k2r/8/8/8/8/8/4q3/R3K2R w KQkq - 0 1");

  ASSERT_FALSE(aurora_test::moveListContains(board, "e1g1"));
  ASSERT_FALSE(aurora_test::moveListContains(board, "e1c1"));
}

TEST(castling, rights_cleared_after_king_move) {
  chess::Board board("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
  chess::Move move = aurora_test::findMove(board, "e1e2");
  ASSERT_TRUE(move.value != 0);
  board.makeMove(move);

  ASSERT_EQ(board.castlingRights & 0x3u, 0u);
}

TEST(castling, rights_cleared_when_rook_captured) {
  chess::Board board("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");
  chess::Move move = aurora_test::findMove(board, "a8a1");
  ASSERT_TRUE(move.value != 0);
  board.makeMove(move);

  ASSERT_EQ(board.castlingRights & 0x2u, 0u); // white queenside gone
  ASSERT_EQ(board.castlingRights & 0x8u, 0u); // black queenside gone (rook moved)
}

TEST(castling, make_move_relocates_rook_kingside) {
  chess::Board board("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
  chess::Move castle = aurora_test::findMove(board, "e1g1");
  ASSERT_EQ(castle.getMoveFlags(), chess::CASTLE);
  board.makeMove(castle);

  ASSERT_EQ(board.findPiece(squareNotationToIndex("g1")), chess::KING);
  ASSERT_EQ(board.findPiece(squareNotationToIndex("f1")), chess::ROOK);
  ASSERT_EQ(board.findPiece(squareNotationToIndex("e1")), chess::null);
  ASSERT_EQ(board.findPiece(squareNotationToIndex("h1")), chess::null);
}

TEST(castling, make_move_relocates_rook_queenside) {
  chess::Board board("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
  chess::Move castle = aurora_test::findMove(board, "e1c1");
  ASSERT_EQ(castle.getMoveFlags(), chess::CASTLE);
  board.makeMove(castle);

  ASSERT_EQ(board.findPiece(squareNotationToIndex("c1")), chess::KING);
  ASSERT_EQ(board.findPiece(squareNotationToIndex("d1")), chess::ROOK);
  ASSERT_EQ(board.findPiece(squareNotationToIndex("a1")), chess::null);
}

TEST(castling, no_castle_when_rights_set_but_rook_missing) {
  // Malformed FEN: KQ rights claimed, but only h1 rook exists
  chess::Board board("4k3/8/8/8/8/8/8/4K2R w KQ - 0 1");

  ASSERT_TRUE(aurora_test::moveListContains(board, "e1g1"));
  ASSERT_FALSE(aurora_test::moveListContains(board, "e1c1"));
  ASSERT_EQ(aurora_test::countMovesWithFlag(board, chess::CASTLE), 1);
}

TEST(castling, black_kingside_and_queenside) {
  chess::Board board("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1");

  ASSERT_TRUE(aurora_test::moveListContains(board, "e8g8"));
  ASSERT_TRUE(aurora_test::moveListContains(board, "e8c8"));
}
