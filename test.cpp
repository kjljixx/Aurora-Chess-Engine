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

  chess::Board board(chess::startPosFen);

  Aurora::options["outputLevel"].value = 3;
  search::timeManagement tm(search::TIME);
  tm.hardLimit = 1000/1000.0;
  tm.limit = 25000/1000.0;

  search::Tree tree;
  search::search(board, tm, tree);




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

  // std::ofstream file("tree.dot");
  // file << search::treeToDOTFormat(tree);
  std::cout << std::endl << search::debugNum;
  std::cout << "Done";
}