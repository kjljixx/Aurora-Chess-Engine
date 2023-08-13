#include "uci.h"

int main() {
  std::string version = "0.0";
  lookupTables::init();
  chess::Board board;
  std::cout << "Aurora " << version << ", a chess engine by kjljixx\n";
  uci::loop(board);
  return 1;
}