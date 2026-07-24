#include "framework.h"
#include "test_helpers.h"
#include "zobrist.h"

TEST(makemove, promotion_creates_queen_and_clears_pawn) {
  chess::Board board("8/4P3/8/8/8/8/8/4k2K w - - 0 1");
  chess::Move promo = aurora_test::findMove(board, "e7e8q");
  ASSERT_EQ(promo.getPromotionPiece(), chess::QUEEN);
  board.makeMove(promo);

  ASSERT_EQ(board.findPiece(squareNotationToIndex("e8")), chess::QUEEN);
  ASSERT_FALSE(board.pawns & (1ULL << squareNotationToIndex("e8")));
  ASSERT_TRUE(board.queens & (1ULL << squareNotationToIndex("e8")));
}

TEST(makemove, underpromotion_to_knight) {
  chess::Board board("8/4P3/8/8/8/8/8/4k2K w - - 0 1");
  ASSERT_TRUE(aurora_test::moveListContains(board, "e7e8n"));
  chess::Move promo = aurora_test::findMove(board, "e7e8n");
  board.makeMove(promo);

  ASSERT_EQ(board.findPiece(squareNotationToIndex("e8")), chess::KNIGHT);
}

TEST(makemove, board_make_and_chess_make_agree) {
  chess::Board a("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
  chess::Board b = a;

  chess::MoveList moves(a);
  ASSERT_TRUE(moves.size() > 0);
  chess::Move move = moves[0];

  a.makeMove(move);
  chess::makeMove(b, move);

  ASSERT_TRUE(aurora_test::boardsEqualFull(a, b));
}

TEST(makemove, board_make_and_chess_make_agree_on_special_moves) {
  struct Case {
    const char* fen;
    const char* uci;
  };
  const Case cases[] = {
    {"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "e1g1"},
    {"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "e1c1"},
    {"8/8/8/2k5/3pP3/8/8/4K3 b - e3 0 1", "d4e3"},
    {"8/4P3/8/8/8/8/8/4k2K w - - 0 1", "e7e8q"},
    {"8/4P3/8/8/8/8/8/4k2K w - - 0 1", "e7e8n"},
  };

  for (const Case& c : cases) {
    chess::Board a(c.fen);
    chess::Board b(c.fen);
    chess::Move move = aurora_test::findMove(a, c.uci);
    ASSERT_TRUE(move.value != 0);

    a.makeMove(move);
    chess::makeMove(b, move);
    ASSERT_TRUE(aurora_test::boardsEqualFull(a, b));
  }
}

TEST(makemove, capture_resets_halfmove_clock) {
  chess::Board board("4k3/8/8/8/8/8/4p3/4K3 w - - 25 1");
  chess::Move capture = aurora_test::findMove(board, "e1e2");
  board.makeMove(capture);
  ASSERT_EQ(board.halfmoveClock, 0);
}

TEST(makemove, quiet_piece_move_increments_halfmove_clock) {
  chess::Board board("4k3/8/8/8/8/8/8/4K3 w - - 5 1");
  chess::Move move = aurora_test::findMove(board, "e1e2");
  board.makeMove(move);
  ASSERT_EQ(board.halfmoveClock, 6);
}

TEST(makemove, mailbox_matches_bitboards_after_moves) {
  chess::Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
  for (chess::Move move : chess::MoveList(board)) {
    chess::Board child = board;
    child.makeMove(move);

    for (int sq = 0; sq < 64; sq++) {
      const chess::Pieces piece = child.findPiece(sq);
      const U64 bit = 1ULL << sq;
      if (piece == chess::null) {
        ASSERT_FALSE(child.occupied & bit);
        continue;
      }
      ASSERT_TRUE(child.occupied & bit);
      switch (piece) {
        case chess::PAWN: ASSERT_TRUE(child.pawns & bit); break;
        case chess::KNIGHT: ASSERT_TRUE(child.knights & bit); break;
        case chess::BISHOP: ASSERT_TRUE(child.bishops & bit); break;
        case chess::ROOK: ASSERT_TRUE(child.rooks & bit); break;
        case chess::QUEEN: ASSERT_TRUE(child.queens & bit); break;
        case chess::KING: ASSERT_TRUE(child.kings & bit); break;
        default: ASSERT_TRUE(false); break;
      }
    }
  }
}
