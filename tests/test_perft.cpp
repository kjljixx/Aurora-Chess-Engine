#include "framework.h"
#include "test_helpers.h"

TEST(perft, start_position_depths_1_to_4) {
  chess::Board board;

  ASSERT_EQ(aurora_test::perft(board, 1), 20ULL);
  ASSERT_EQ(aurora_test::perft(board, 2), 400ULL);
  ASSERT_EQ(aurora_test::perft(board, 3), 8902ULL);
  ASSERT_EQ(aurora_test::perft(board, 4), 197281ULL);
}

// Kiwipete — heavy on castling, EP, and promotions
TEST(perft, kiwipete_depths_1_to_3) {
  chess::Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

  ASSERT_EQ(aurora_test::perft(board, 1), 48ULL);
  ASSERT_EQ(aurora_test::perft(board, 2), 2039ULL);
  ASSERT_EQ(aurora_test::perft(board, 3), 97862ULL);
}

TEST(perft, position3_depths_1_to_4) {
  chess::Board board("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");

  ASSERT_EQ(aurora_test::perft(board, 1), 14ULL);
  ASSERT_EQ(aurora_test::perft(board, 2), 191ULL);
  ASSERT_EQ(aurora_test::perft(board, 3), 2812ULL);
  ASSERT_EQ(aurora_test::perft(board, 4), 43238ULL);
}

// Promotions and underpromotions
TEST(perft, position4_depths_1_to_3) {
  chess::Board board("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");

  ASSERT_EQ(aurora_test::perft(board, 1), 6ULL);
  ASSERT_EQ(aurora_test::perft(board, 2), 264ULL);
  ASSERT_EQ(aurora_test::perft(board, 3), 9467ULL);
}

TEST(perft, position5_depths_1_to_3) {
  chess::Board board("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");

  ASSERT_EQ(aurora_test::perft(board, 1), 44ULL);
  ASSERT_EQ(aurora_test::perft(board, 2), 1486ULL);
  ASSERT_EQ(aurora_test::perft(board, 3), 62379ULL);
}

TEST(perft, position6_depths_1_to_3) {
  chess::Board board("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");

  ASSERT_EQ(aurora_test::perft(board, 1), 46ULL);
  ASSERT_EQ(aurora_test::perft(board, 2), 2079ULL);
  ASSERT_EQ(aurora_test::perft(board, 3), 89890ULL);
}
