#include "framework.h"
#include "test_helpers.h"
#include "evaluation.h"

TEST(nnue, update_accumulator_matches_refresh) {
  chess::Board board("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");

  int checked = 0;
  for (chess::Move move : chess::MoveList(board)) {
    if (checked >= 12) {
      break;
    }

    chess::Board child = board;
    evaluation::NNUE<evaluation::NNUEhiddenNeurons> incremental(evaluation::nnueParameters);
    evaluation::NNUE<evaluation::NNUEhiddenNeurons> refreshed(evaluation::nnueParameters);
    incremental.refreshAccumulator(child);

    incremental.updateAccumulator(child, move);
    refreshed.refreshAccumulator(child);

    for (int side = 0; side < 2; side++) {
      for (int i = 0; i < evaluation::NNUEhiddenNeurons; i++) {
        ASSERT_EQ(incremental.accumulator[side][i], refreshed.accumulator[side][i]);
      }
    }
    checked++;
  }
}

TEST(nnue, update_matches_refresh_on_ep_castle_promo) {
  struct Case {
    const char* fen;
    const char* uci;
  };
  const Case cases[] = {
    {"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", "e1g1"},
    {"8/8/8/2k5/3pP3/8/8/4K3 b - e3 0 1", "d4e3"},
    {"8/4P3/8/8/8/8/8/4k2K w - - 0 1", "e7e8q"},
  };

  for (const Case& c : cases) {
    chess::Board board(c.fen);
    evaluation::NNUE<evaluation::NNUEhiddenNeurons> incremental(evaluation::nnueParameters);
    evaluation::NNUE<evaluation::NNUEhiddenNeurons> refreshed(evaluation::nnueParameters);
    incremental.refreshAccumulator(board);

    chess::Move move = aurora_test::findMove(board, c.uci);
    ASSERT_TRUE(move.value != 0);

    incremental.updateAccumulator(board, move);
    refreshed.refreshAccumulator(board);

    for (int side = 0; side < 2; side++) {
      for (int i = 0; i < evaluation::NNUEhiddenNeurons; i++) {
        ASSERT_EQ(incremental.accumulator[side][i], refreshed.accumulator[side][i]);
      }
    }
  }
}
