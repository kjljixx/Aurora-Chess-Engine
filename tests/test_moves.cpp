#include "framework.h"
#include "test_helpers.h"

TEST(moves, promotion_flag_decoding) {
  chess::Move q(12, 4, chess::PROMOTION, chess::QUEEN);
  chess::Move n(12, 4, chess::PROMOTION, chess::KNIGHT);
  chess::Move b(12, 4, chess::PROMOTION, chess::BISHOP);
  chess::Move r(12, 4, chess::PROMOTION, chess::ROOK);

  ASSERT_EQ(q.getMoveFlags(), chess::PROMOTION);
  ASSERT_EQ(q.getPromotionPiece(), chess::QUEEN);
  ASSERT_EQ(n.getPromotionPiece(), chess::KNIGHT);
  ASSERT_EQ(b.getPromotionPiece(), chess::BISHOP);
  ASSERT_EQ(r.getPromotionPiece(), chess::ROOK);
  ASSERT_EQ(q.toStringRep(), "e2e1q");
}

TEST(moves, castle_and_ep_flags_distinct_from_promotion) {
  chess::Move castle(4, 6, chess::CASTLE);
  chess::Move ep(35, 42, chess::ENPASSANT);

  ASSERT_EQ(castle.getMoveFlags(), chess::CASTLE);
  ASSERT_EQ(ep.getMoveFlags(), chess::ENPASSANT);
  ASSERT_FALSE(castle.getMoveFlags() == chess::PROMOTION);
  ASSERT_FALSE(ep.getMoveFlags() == chess::PROMOTION);
}

TEST(moves, pinned_knight_has_no_legal_moves) {
  // Knight on e4 pinned by rook on e8 to king on e1
  chess::Board board("4r3/8/8/8/4N3/8/8/4K3 w - - 0 1");
  for (chess::Move move : chess::MoveList(board)) {
    ASSERT_FALSE(move.getStartSquare() == squareNotationToIndex("e4"));
  }
}

TEST(moves, pinned_bishop_can_move_along_pin_ray) {
  // King a1, bishop b2, enemy bishop h8 — diagonal pin; bishop may slide along the ray
  chess::Board diagonal("7b/8/8/8/8/8/1B6/K7 w - - 0 1");
  bool alongRay = aurora_test::moveListContains(diagonal, "b2c3")
      || aurora_test::moveListContains(diagonal, "b2d4")
      || aurora_test::moveListContains(diagonal, "b2e5")
      || aurora_test::moveListContains(diagonal, "b2f6")
      || aurora_test::moveListContains(diagonal, "b2g7")
      || aurora_test::moveListContains(diagonal, "b2h8");
  ASSERT_TRUE(alongRay);
}

TEST(moves, double_check_only_king_moves) {
  // King e1 checked by rook e8 and bishop a5
  chess::Board board("4r3/8/8/b7/8/8/8/4K3 w - - 0 1");
  chess::MoveList moves(board);
  for (chess::Move move : moves) {
    ASSERT_EQ(board.findPiece(move.getStartSquare()), chess::KING);
  }
  ASSERT_TRUE(moves.size() > 0);
}

TEST(moves, check_must_be_resolved) {
  chess::Board board("4k3/8/8/8/8/8/4r3/4K3 w - - 0 1");
  // King in check from rook on e2; only king moves or capture/block
  for (chess::Move move : chess::MoveList(board)) {
    chess::Board child = board;
    child.makeMove(move);
    // After our move, side is black — white king must not be attackable by black
    child.sideToMove = chess::WHITE;
    ASSERT_FALSE(child.squareUnderAttack(bitscanForward(child.kings & child.white)) <= 63);
  }
}

TEST(moves, square_attackers_finds_pawn) {
  chess::Board board("4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1");
  const uint8_t e4 = squareNotationToIndex("e4");
  const U64 attackers = board.squareAttackers(e4, chess::BLACK);
  ASSERT_TRUE(attackers & (1ULL << squareNotationToIndex("d5")));
}

TEST(moves, square_attackers_independent_of_side_to_move) {
  chess::Board board("4k3/8/8/3p4/4P3/8/8/4K3 b - - 0 1");
  const uint8_t e4 = squareNotationToIndex("e4");
  const U64 attackers = board.squareAttackers(e4, chess::BLACK);
  ASSERT_TRUE(attackers & (1ULL << squareNotationToIndex("d5")));
}

namespace {
bool hasMoveTo(chess::MoveList moves, const char* square) {
  for (chess::Move move : moves) {
    if (move.getEndSquare() == squareNotationToIndex(square)) return true;
  }
  return false;
}
}

TEST(moves, captures_include_quiet_queen_promotion_only) {
  chess::Board board("7k/4P3/8/8/8/8/8/4K3 w - - 0 1");
  chess::MoveList captures(board, true);
  ASSERT_EQ(captures.size(), 1u);
  ASSERT_EQ(captures[0].getMoveFlags(), chess::PROMOTION);
  ASSERT_EQ(captures[0].getPromotionPiece(), chess::QUEEN);
  ASSERT_EQ(captures[0].getEndSquare(), squareNotationToIndex("e8"));
}

TEST(moves, captures_keep_all_promotions_for_capture_plus_one_queen_push) {
  chess::Board board("3r3k/4P3/8/8/8/8/8/4K3 w - - 0 1");
  ASSERT_EQ(chess::MoveList(board, true).size(), 5u);
}

TEST(moves, captures_skip_blocked_promotion_push) {
  chess::Board board("4n2k/4P3/8/8/8/8/8/4K3 w - - 0 1");
  ASSERT_EQ(chess::MoveList(board, true).size(), 0u);
}

TEST(moves, captures_skip_promotion_push_that_ignores_check) {
  chess::Board board("7k/4P3/8/8/8/8/4r3/4K3 w - - 0 1");
  ASSERT_FALSE(hasMoveTo(chess::MoveList(board, true), "e8"));
}

TEST(moves, captures_skip_promotion_push_of_bishop_pinned_pawn) {
  chess::Board board("3b3k/4P3/8/6K1/8/8/8/8 w - - 0 1");
  ASSERT_FALSE(hasMoveTo(chess::MoveList(board, true), "e8"));
}
