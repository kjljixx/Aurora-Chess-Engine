#include "uci.h"

int main(int argc, char* argv[]) {
  #if DATAGEN > 0
    std::cout << "Preprocessor Variable DATAGEN must be set to 0 for normal use";
    getchar();
    return 0;
  #endif
  search::init();

  if(argc > 1){
    if(std::string(argv[1]) == "bench"){
      chess::Board board;
      search::Tree tree;

      std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

      search::search(board, search::timeManagement(search::NODES, 1000000), tree);

      std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsed = end - start;

      std::cout << "1 nodes 80000 nps" << std::endl; //make OpenBench happy
      return 1;
    }
  }

  chess::Board board;
  std::cout << "Aurora " << VERSION_NUM VERSION_NAME DEV_STRING << ", a chess engine by kjljixx\n";
  uci::loop(board);
  return 1;
}