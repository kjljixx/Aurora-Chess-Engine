#include "framework.h"
#include "bitboards.h"

TEST(bitboards, square_notation_roundtrip) {
  ASSERT_EQ(squareIndexToNotation(0), "a1");
  ASSERT_EQ(squareIndexToNotation(56), "a8");
  ASSERT_EQ(squareIndexToNotation(63), "h8");
  ASSERT_EQ(squareNotationToIndex("e4"), 28u);
  ASSERT_EQ(squareNotationToIndex("h1"), 7u);
}

TEST(bitboards, square_rank_and_file) {
  ASSERT_EQ(squareIndexToRank(0), 0u);
  ASSERT_EQ(squareIndexToFile(0), 0u);
  ASSERT_EQ(squareIndexToRank(63), 7u);
  ASSERT_EQ(squareIndexToFile(63), 7u);
  ASSERT_EQ(squareIndexToRank(28), 3u);
  ASSERT_EQ(squareIndexToFile(28), 4u);
}

TEST(bitboards, popcount_and_bitscan) {
  ASSERT_EQ(popCount(0ULL), 0u);
  ASSERT_EQ(popCount(0xFFULL), 8u);
  ASSERT_EQ(bitscanForward(0x1000ULL), 12u);
  ASSERT_EQ(bitscanReverse(0x1000ULL), 12u);
}

TEST(bitboards, pop_lsb) {
  U64 board = 0x5000ULL;
  ASSERT_EQ(popLsb(board), 12u);
  ASSERT_EQ(board, 0x4000ULL);
  ASSERT_EQ(popLsb(board), 14u);
  ASSERT_EQ(board, 0ULL);
}
