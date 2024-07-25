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
      std::cout << "1 nodes 1 nps" << std::endl; //make OpenBench happy
    }
  }

  chess::Board board;
  std::cout << "Aurora " << VERSION_NUM VERSION_NAME DEV_STRING << ", a chess engine by kjljixx\n";
  uci::loop(board);
  return 1;
}