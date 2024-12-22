#include "search.h"

std::string treeToDOTFormat(search::Tree &tree) {
  std::string s = "digraph {\n";

  for (int i = 0; i < tree.tree.size(); i++) {
    if(!(tree.tree[i].visits >= 0)){continue;}
    s += std::to_string(U64(&tree.tree[i])) + "[label=\"" + std::to_string(tree.tree[i].avgValue) + "\"] ;\n";
    for(auto edge : tree.tree[i].children){
      if(!(edge.child && edge.child->visits >= 0)){continue;}
      s += std::to_string(U64(&tree.tree[i])) + " -> " + std::to_string(U64(edge.child)) + "[label=\"" + edge.edge.toStringRep() + "," + std::to_string(edge.value) + "\"] ;\n";
    }
  }

  s += "}\n";

  return s;
}

int main() {
  search::init();

  chess::Board board("8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1");

  Aurora::options["outputLevel"].value = 3;
  search::timeManagement tm(search::NODES, 5000000);

  search::Tree tree;
  search::search(board, tm, tree);

  chess::Board testBoard("8/1k6/3p4/p2P1p2/P2P1P2/8/2K5/8 w - - 8 5");
  U64 testHash = zobrist::getHash(testBoard);
  std::cout << tree.getTTEntry(testHash)->node->avgValue << "\n";


  // search::Tree tree;
  // tree.push_back(search::Node()); tree.root = &tree.tree[0]; std::cout << tree.root;
  // tree.push_back(search::Node());
  // tree.push_back(search::Node());
  // tree.root = &tree.tree[tree.tree.size()-1];
  // tree.tree[0].children = {search::Edge(&tree.tree[1], chess::Move(), -0.8), search::Edge(nullptr, chess::Move(1), 0.5)};
  // tree.tree[1].children = {search::Edge(&tree.tree[2], chess::Move(), -0.8), search::Edge(nullptr, chess::Move(1), 0.5)};
  // tree.tree[2].children = {search::Edge(tree.root, chess::Move(), -0.8), search::Edge(nullptr, chess::Move(1), 0.5)};
  // std::vector<std::pair<search::Edge*, U64>> traversePath = {{&tree.tree[0].children[0], 0}, {&tree.tree[1].children[0], 0}, {&tree.tree[2].children[0], 0}};
  // search::loopHandling(traversePath, 0, 2);
  // search::loopRepetitionDetection(traversePath, 0, 2);

  std::ofstream file("tree.dot");
  file << search::treeToDOTFormat(tree);
  std::cout << "Done";
}