#include "framework.h"
#include "chess.h"

TEST(fen, start_position_fields) {
  chess::Board board;
  ASSERT_EQ(board.sideToMove, chess::WHITE);
  ASSERT_EQ(board.castlingRights, 0xFu);
  ASSERT_EQ(board.enPassant, 0ULL);
  ASSERT_EQ(popCount(board.white), 16u);
  ASSERT_EQ(popCount(board.black), 16u);
}

TEST(fen, set_fen_restores_piece_placement) {
  const std::string fen = "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1";
  chess::Board board(fen);

  ASSERT_EQ(board.sideToMove, chess::BLACK);
  ASSERT_TRUE(board.pawns & (1ULL << squareNotationToIndex("e4")));
  ASSERT_TRUE(board.white & (1ULL << squareNotationToIndex("e4")));
  ASSERT_EQ(board.enPassant, 1ULL << squareNotationToIndex("e3"));
  ASSERT_EQ(board.castlingRights, 0xFu);
}

TEST(fen, get_fen_contains_board_state) {
  chess::Board board(chess::startPosFen);
  const std::string fen = board.getFen();

  ASSERT_TRUE(fen.find("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR") != std::string::npos);
  ASSERT_TRUE(fen.find(" w ") != std::string::npos);
  ASSERT_TRUE(fen.find("KQkq") != std::string::npos);
}

TEST(fen, roundtrip_preserves_core_fields) {
  const std::string original = "rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2";
  chess::Board board(original);
  chess::Board again(board.getFen());

  ASSERT_EQ(board.sideToMove, again.sideToMove);
  ASSERT_EQ(board.castlingRights, again.castlingRights);
  ASSERT_EQ(board.enPassant, again.enPassant);
  ASSERT_EQ(board.halfmoveClock, again.halfmoveClock);
  ASSERT_EQ(board.pawns, again.pawns);
  ASSERT_EQ(board.knights, again.knights);
  ASSERT_EQ(board.bishops, again.bishops);
  ASSERT_EQ(board.rooks, again.rooks);
  ASSERT_EQ(board.queens, again.queens);
  ASSERT_EQ(board.kings, again.kings);
  ASSERT_EQ(board.white, again.white);
  ASSERT_EQ(board.black, again.black);
  ASSERT_TRUE(board.mailbox == again.mailbox);
}

TEST(fen, empty_castling_and_ep) {
  chess::Board board("4k3/8/8/8/8/8/8/4K3 w - - 12 30");
  ASSERT_EQ(board.castlingRights, 0u);
  ASSERT_EQ(board.enPassant, 0ULL);
  ASSERT_EQ(board.halfmoveClock, 12);
  ASSERT_TRUE(board.getFen().find(" w - - 12 ") != std::string::npos);
}

TEST(fen, operator_eq_ignores_castling_and_ep) {
  chess::Board a("4k3/8/8/8/8/8/8/4K3 w KQ - 0 1");
  chess::Board b("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
  ASSERT_TRUE(a == b);
  ASSERT_FALSE(a.castlingRights == b.castlingRights);
}

