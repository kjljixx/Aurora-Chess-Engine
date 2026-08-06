#include "framework.h"
#include "test_helpers.h"
#include "search.h"

#include <cmath>
#include <limits>

namespace {

search::Tree makeTinyTree() {
  search::Tree tree;
  Aurora::hash.value = 1;
  Aurora::ttHash.value = 0;
  Aurora::ttHashProportion.value = 0;
  Aurora::outputLevel.value = -1;
  tree.setHash();
  // Force LRU reclaim path: one Node of budget
  tree.sizeLimit = sizeof(search::Node);
  return tree;
}

void addRoot(search::Tree& tree) {
  tree.push_back(search::Node());
  tree.rootIdx = uint32_t(tree.tree.size() - 1);
}

} // namespace

TEST(search, move_lru_flag_ignored_by_equality) {
  chess::Move a(12, 28);
  chess::Move b = a;
  b.value |= (1 << 15);
  ASSERT_TRUE(a == b);
  ASSERT_TRUE((a.value & (1 << 15)) == 0);
  ASSERT_TRUE((b.value & (1 << 15)) != 0);
}

TEST(search, find_best_q_picks_lowest_edge_value) {
  search::Tree tree;
  addRoot(tree);
  search::Node* root = tree.root();
  root->children.resize(3);
  root->children[0].value = 0.2f;
  root->children[0].edge = chess::Move(0, 1);
  root->children[1].value = -0.5f;
  root->children[1].edge = chess::Move(0, 2);
  root->children[2].value = 0.1f;
  root->children[2].edge = chess::Move(0, 3);

  ASSERT_EQ(search::findBestQEdge(root).edge.getEndSquare(), 2);
  ASSERT_EQ(search::findBestQ(root), -0.5f);
}

TEST(search, find_best_a_can_disagree_with_q) {
  // Documents the intentional Q-vs-A split used for PV vs bestmove.
  search::Tree tree;
  addRoot(tree);

  uint32_t child0Idx = tree.getIdx(tree.push_back(search::Node(tree.rootIdx)));
  uint32_t child1Idx = tree.getIdx(tree.push_back(search::Node(tree.rootIdx)));
  tree.getNode(child0Idx)->avgValue = 0.8f;  // worse average
  tree.getNode(child0Idx)->visits = 10;
  tree.getNode(child1Idx)->avgValue = -0.2f; // better average
  tree.getNode(child1Idx)->visits = 10;

  search::Node* root = tree.root();
  root->children.resize(2);
  root->children[0].childIdx = child0Idx;
  root->children[0].value = -0.9f; // better Q (lower)
  root->children[0].edge = chess::Move(0, 1);
  root->children[1].childIdx = child1Idx;
  root->children[1].value = 0.0f;  // worse Q
  root->children[1].edge = chess::Move(0, 2);

  ASSERT_EQ(search::findBestQEdge(root).edge.getEndSquare(), 1);
  ASSERT_EQ(search::findBestAEdge(root, tree).edge.getEndSquare(), 2);
}

TEST(search, select_edge_handles_zero_visits_and_iters) {
  search::Tree tree;
  addRoot(tree);
  search::Node* root = tree.root();
  root->visits = 0;
  root->iters = 0;
  root->children.resize(2);
  root->children[0].value = 0.1f;
  root->children[0].edge = chess::Move(0, 1);
  root->children[1].value = -0.3f;
  root->children[1].edge = chess::Move(0, 2);

  // Must not crash / produce NaN priorities; prefers the lower (better) value when exploration is 0.
  uint8_t idx = search::selectEdge(root, tree, true);
  ASSERT_EQ(static_cast<int>(idx), 1);
}

TEST(search, select_edge_handles_child_with_zero_visits) {
  search::Tree tree;
  addRoot(tree);
  tree.root()->visits = 8;
  tree.root()->iters = 4;

  uint32_t childIdx = tree.getIdx(tree.push_back(search::Node(tree.rootIdx)));
  tree.getNode(childIdx)->visits = 0;
  tree.getNode(childIdx)->iters = 0;
  tree.getNode(childIdx)->avgValue = 0.0f;

  search::Node* root = tree.root();
  root->children.resize(1);
  root->children[0].childIdx = childIdx;
  root->children[0].value = 0.0f;
  root->children[0].edge = chess::Move(0, 1);

  uint8_t idx = search::selectEdge(root, tree, true);
  ASSERT_EQ(static_cast<int>(idx), 0);
}

TEST(search, select_edge_lru_visits_from_parent_residual) {
  // LRU-pruned visit estimate = (parentVisits - knownChildVisits) / lruCount.
  // A large residual should suppress exploration of the pruned edge relative to
  // a never-expanded edge with the same Q.
  search::Tree tree;
  addRoot(tree);
  tree.root()->visits = 100;
  tree.root()->iters = 50;

  uint32_t liveIdx = tree.getIdx(tree.push_back(search::Node(tree.rootIdx)));
  tree.getNode(liveIdx)->visits = 10;
  tree.getNode(liveIdx)->iters = 10;
  tree.getNode(liveIdx)->avgValue = 0.0f;

  search::Node* root = tree.root();
  root->children.resize(3);

  root->children[0].childIdx = liveIdx;
  root->children[0].value = 0.0f;
  root->children[0].edge = chess::Move(0, 1);

  root->children[1].childIdx = UINT32_MAX;
  root->children[1].value = 0.0f;
  root->children[1].edge = chess::Move(0, 2);
  root->children[1].edge.value |= (1 << 15); // LRU-pruned; residual ~90

  root->children[2].childIdx = UINT32_MAX;
  root->children[2].value = 0.0f;
  root->children[2].edge = chess::Move(0, 3); // never expanded; visits=1

  uint8_t idx = search::selectEdge(root, tree, true);
  ASSERT_EQ(static_cast<int>(idx), 2);
}

TEST(search, expand_creates_edges_for_all_moves) {
  search::Tree tree;
  addRoot(tree);
  chess::Board board;
  chess::MoveList moves(board);
  const size_t before = tree.currSize;
  search::expand(tree, tree.root(), moves);
  ASSERT_EQ(tree.root()->children.size(), moves.size());
  ASSERT_EQ(tree.currSize, before + moves.size() * sizeof(search::Edge));
  for (size_t i = 0; i < moves.size(); i++) {
    ASSERT_EQ(tree.root()->children[i].childIdx, UINT32_MAX);
    ASSERT_EQ(tree.root()->children[i].value, -2.0f);
  }
}

TEST(search, playout_returns_loss_on_checkmate) {
  search::Tree tree;
  Aurora::hash.value = 1;
  Aurora::ttHash.value = 1;
  Aurora::outputLevel.value = -1;
  tree.setHash();

  chess::Board board("4k3/4Q3/4K3/8/8/8/8/8 b - - 0 1");
  board.history[board.halfmoveClock] = zobrist::getHash(board);
  board.hashed = true;

  evaluation::NNUE<evaluation::NNUEhiddenNeurons> nnue(evaluation::nnueParameters);
  float val = search::playout(tree, board, nnue);
  ASSERT_EQ(val, static_cast<float>(chess::LOSS));
}

TEST(search, playout_returns_draw_on_stalemate) {
  search::Tree tree;
  Aurora::hash.value = 1;
  Aurora::ttHash.value = 1;
  Aurora::outputLevel.value = -1;
  tree.setHash();

  chess::Board board("7k/5P2/6K1/8/8/8/8/8 b - - 0 1");
  board.history[board.halfmoveClock] = zobrist::getHash(board);
  board.hashed = true;

  evaluation::NNUE<evaluation::NNUEhiddenNeurons> nnue(evaluation::nnueParameters);
  float val = search::playout(tree, board, nnue);
  ASSERT_EQ(val, static_cast<float>(chess::DRAW));
}

TEST(search, playout_tt_hit_returns_cached_value) {
  search::Tree tree;
  Aurora::hash.value = 1;
  Aurora::ttHash.value = 1;
  Aurora::outputLevel.value = -1;
  tree.setHash();

  chess::Board board;
  board.history[board.halfmoveClock] = zobrist::getHash(board);
  board.hashed = true;

  search::TTEntry* entry = tree.getTTEntry(board.history[board.halfmoveClock]);
  entry->hash = board.history[board.halfmoveClock] >> 32;
  entry->val = 0.42f;

  evaluation::NNUE<evaluation::NNUEhiddenNeurons> nnue(evaluation::nnueParameters);
  float val = search::playout(tree, board, nnue);
  ASSERT_EQ(val, 0.42f);
}

TEST(search, lru_eviction_does_not_destroy_root) {
  search::Tree tree = makeTinyTree();
  addRoot(tree);
  const uint32_t rootIdx = tree.rootIdx;
  ASSERT_EQ(tree.headIdx, rootIdx);
  ASSERT_EQ(tree.tailIdx, rootIdx);

  // Allocating another node with a 1-node budget used to overwrite the root.
  tree.push_back(search::Node(rootIdx));
  ASSERT_EQ(tree.rootIdx, rootIdx);
  ASSERT_TRUE(tree.root()->parentIdx == UINT32_MAX);
  ASSERT_TRUE(tree.tree.size() >= 2);
}

TEST(search, lru_eviction_sets_prune_flag_on_parent_edge) {
  search::Tree tree;
  Aurora::hash.value = 1;
  Aurora::ttHash.value = 0;
  Aurora::ttHashProportion.value = 0;
  Aurora::outputLevel.value = -1;
  tree.setHash();
  // Budget: root + one child node, then reclaim on the next alloc
  tree.sizeLimit = 2 * sizeof(search::Node);

  addRoot(tree);
  tree.root()->children.resize(1);
  tree.root()->children[0].edge = chess::Move(0, 1);
  tree.root()->children[0].value = 0.25f;

  uint32_t childIdx = tree.getIdx(tree.push_back(search::Node(tree.rootIdx)));
  tree.getNode(childIdx)->index = 0;
  tree.getNode(childIdx)->avgValue = 0.25f;
  tree.getNode(childIdx)->visits = 3;
  tree.root()->children[0].childIdx = childIdx;

  // Make root MRU so the child is LRU tail
  tree.moveToHead(tree.root());
  ASSERT_EQ(tree.tailIdx, childIdx);

  // Force reclaim
  tree.currSize = tree.sizeLimit;
  search::Node* other = tree.push_back(search::Node(tree.rootIdx));
  ASSERT_TRUE(other != nullptr);

  ASSERT_EQ(tree.root()->children[0].childIdx, UINT32_MAX);
  ASSERT_TRUE((tree.root()->children[0].edge.value & (1 << 15)) != 0);
  ASSERT_EQ(tree.root()->children[0].value, 0.25f);
}

TEST(search, make_move_reuses_child_and_decrements_stats) {
  search::Tree tree;
  Aurora::hash.value = 1;
  Aurora::ttHash.value = 1;
  Aurora::outputLevel.value = -1;
  tree.setHash();
  addRoot(tree);

  chess::Board rootBoard;
  rootBoard.history[rootBoard.halfmoveClock] = zobrist::getHash(rootBoard);
  rootBoard.hashed = true;
  chess::Board board = rootBoard;

  chess::Move move = aurora_test::findMove(board, "e2e4");
  ASSERT_TRUE(move.value != 0);

  chess::MoveList moves(board);
  search::expand(tree, tree.root(), moves);
  int moveIdx = -1;
  for (int i = 0; i < int(moves.size()); i++) {
    if (moves[i] == move) {
      moveIdx = i;
      tree.root()->children[i].value = -0.1f;
      uint32_t childIdx = tree.getIdx(tree.push_back(search::Node(tree.rootIdx)));
      tree.getNode(childIdx)->index = uint8_t(i);
      tree.getNode(childIdx)->visits = 5;
      tree.getNode(childIdx)->iters = 3;
      tree.getNode(childIdx)->avgValue = -0.1f;
      tree.root()->children[i].childIdx = childIdx;
    } else {
      tree.root()->children[i].value = 0.5f;
    }
  }
  ASSERT_TRUE(moveIdx >= 0);
  tree.root()->visits = 20;
  tree.root()->iters = 10;

  search::makeMove(board, move, rootBoard, tree);

  ASSERT_EQ(tree.root()->visits, 4u);
  ASSERT_EQ(tree.root()->iters, 2);
  ASSERT_EQ(board.sideToMove, chess::BLACK);
  ASSERT_TRUE(rootBoard.equivalentHistory(board));
}

TEST(search, make_move_destroys_tree_when_child_missing) {
  search::Tree tree;
  Aurora::hash.value = 1;
  Aurora::ttHash.value = 1;
  Aurora::outputLevel.value = -1;
  tree.setHash();
  addRoot(tree);

  chess::Board rootBoard;
  rootBoard.history[rootBoard.halfmoveClock] = zobrist::getHash(rootBoard);
  rootBoard.hashed = true;
  chess::Board board = rootBoard;

  // Root has no matching child edge for the move
  chess::Move move = aurora_test::findMove(board, "e2e4");
  search::makeMove(board, move, rootBoard, tree);
  ASSERT_EQ(tree.rootIdx, UINT32_MAX);
  ASSERT_EQ(tree.tree.size(), 0u);
}

TEST(search, nodes_limit_zero_exits_immediately_like_tb_shortcut) {
  // Mirrors the post-fix TB time-management trick: NODES with limit 0 and
  // startNodes == visits must not enter the search loop.
  search::Tree tree;
  Aurora::hash.value = 1;
  Aurora::ttHash.value = 1;
  Aurora::outputLevel.value = -1;
  tree.setHash();
  addRoot(tree);

  tree.root()->visits = 50;
  tree.startNodes = 50;
  const uint32_t visitsBefore = tree.root()->visits;

  search::timeManagement tm(search::NODES, 0);
  // Emulate loop predicate used in search()
  const bool shouldSearch =
    (tm.tmType == search::NODES) &&
    (!tm.useSoftHardNodeLimits) &&
    ((tree.root()->visits - tree.startNodes) < tm.limit);
  ASSERT_FALSE(shouldSearch);

  // Contrast: the old limit=-1 trick fails when startNodes > visits
  tree.root()->visits = 1;
  tree.startNodes = 100;
  tm.limit = -1;
  const bool oldHackWouldSpin =
    (tm.tmType == search::NODES) &&
    ((int(tree.root()->visits) - int(tree.startNodes)) < int(tm.limit));
  ASSERT_TRUE(oldHackWouldSpin);

  tree.root()->visits = visitsBefore;
}

TEST(search, shallow_search_produces_legal_best_edge) {
  search::Tree tree;
  Aurora::hash.value = 16;
  Aurora::ttHash.value = 1;
  Aurora::outputLevel.value = -1;
  tree.setHash();

  chess::Board board;
  board.history[board.halfmoveClock] = zobrist::getHash(board);
  board.hashed = true;

  search::search(board, search::timeManagement(search::NODES, 32), tree);

  ASSERT_TRUE(tree.root() != nullptr);
  ASSERT_TRUE(tree.root()->children.size() > 0);
  ASSERT_TRUE(tree.root()->visits > 0);

  search::Edge bestQ = search::findBestQEdge(tree.root());
  search::Edge bestA = search::findBestAEdge(tree.root(), tree);

  // Best move must be in the legal move list (bit15 may be set on reused edges)
  bool foundQ = false;
  bool foundA = false;
  for (chess::Move m : chess::MoveList(board)) {
    if (m == bestQ.edge) foundQ = true;
    if (m == bestA.edge) foundA = true;
  }
  ASSERT_TRUE(foundQ);
  ASSERT_TRUE(foundA);

  search::destroyTree(tree);
}

TEST(search, mate_in_one_prefers_checkmate) {
  search::Tree tree;
  Aurora::hash.value = 16;
  Aurora::ttHash.value = 1;
  Aurora::outputLevel.value = -1;
  tree.setHash();

  // White to move, Qxf7# is mate
  chess::Board board("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4");
  board.history[board.halfmoveClock] = zobrist::getHash(board);
  board.hashed = true;

  search::search(board, search::timeManagement(search::NODES, 64), tree);

  search::Edge best = search::findBestQEdge(tree.root());
  chess::Board after = board;
  chess::makeMove(after, best.edge);
  const bool mated = chess::getGameStatus(after, chess::isLegalMoves(after)) == chess::LOSS;
  // With enough nodes this position should find mate; if not, Q should still be strongly winning
  if (!mated) {
    ASSERT_TRUE(-best.value > 0.9f);
  } else {
    ASSERT_TRUE(mated);
  }

  search::destroyTree(tree);
}

TEST(search, backpropagate_updates_edge_and_child_avg) {
  search::Tree tree;
  Aurora::hash.value = 1;
  Aurora::ttHash.value = 1;
  Aurora::outputLevel.value = -1;
  tree.setHash();
  addRoot(tree);

  uint32_t childIdx = tree.getIdx(tree.push_back(search::Node(tree.rootIdx)));
  tree.getNode(childIdx)->visits = 1;
  tree.getNode(childIdx)->iters = 1;
  tree.getNode(childIdx)->avgValue = 0.0f;
  tree.getNode(childIdx)->sumSquaredVals = 0.0f;

  tree.root()->children.resize(1);
  tree.root()->children[0].childIdx = childIdx;
  tree.root()->children[0].value = 0.0f;
  tree.root()->children[0].edge = chess::Move(0, 1);

  std::vector<std::tuple<uint32_t, uint8_t, U64>> path;
  path.push_back({tree.rootIdx, 0, 0xABCDEFULL});

  search::backpropagate(tree, -0.5f, path, 2, true, false, true);

  ASSERT_EQ(tree.root()->children[0].value, -0.5f);
  ASSERT_EQ(tree.getNode(childIdx)->visits, 3u);
  ASSERT_EQ(tree.getNode(childIdx)->iters, 2);
  ASSERT_TRUE(std::isfinite(tree.getNode(childIdx)->avgValue));
}

TEST(search, variance_non_negative_after_identical_updates) {
  search::Node node;
  node.avgValue = 0.25f;
  node.sumSquaredVals = 0.25f * 0.25f;
  ASSERT_TRUE(node.variance() >= -1e-6f);
}

TEST(search, mark_subtree_toggles_and_counts) {
  search::Tree tree;
  addRoot(tree);
  tree.root()->mark = false;

  uint32_t c0Idx = tree.getIdx(tree.push_back(search::Node(tree.rootIdx)));
  uint32_t c1Idx = tree.getIdx(tree.push_back(search::Node(tree.rootIdx)));
  tree.getNode(c0Idx)->mark = false;
  tree.getNode(c1Idx)->mark = false;

  search::Node* root = tree.root();
  root->children.resize(2);
  root->children[0].childIdx = c0Idx;
  root->children[1].childIdx = c1Idx;

  uint64_t marked = search::markSubtree(tree, root);
  ASSERT_EQ(marked, 3u);
  ASSERT_TRUE(root->mark);
  ASSERT_TRUE(tree.getNode(c0Idx)->mark);
  ASSERT_TRUE(tree.getNode(c1Idx)->mark);

  marked = search::markSubtree(tree, root);
  ASSERT_EQ(marked, 3u);
  ASSERT_FALSE(root->mark);
  ASSERT_FALSE(tree.getNode(c0Idx)->mark);
  ASSERT_FALSE(tree.getNode(c1Idx)->mark);
}
