#include "uci.h"

int main() {
  #if DATAGEN > 0
    std::cout << "Preprocessor Variable DATAGEN must be set to 0 for normal use";
    getchar();
    return 0;
  #endif
  search::init();
  //tb_init("C:\\Users\\kjlji\\OneDrive\\Documents\\VSCode\\C++\\AuroraChessEngine-main\\3-4-5");

  chess::Board board;
  std::cout << "Aurora " << VERSION << ", a chess engine by kjljixx\n";
  uci::loop(board);
  return 1;
}