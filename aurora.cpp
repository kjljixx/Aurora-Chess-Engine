#include "uci.h"

int main() {
  std::string version = "0.8-passedpawneval";
  search::init();
  //tb_init("C:\\Users\\kjlji\\OneDrive\\Documents\\VSCode\\C++\\AuroraChessEngine-main\\3-4-5");

  chess::Board board;
  std::cout << "Aurora " << version << ", a chess engine by kjljixx\n";
  uci::loop(board);
  return 1;
}