#include "uci.h"
#include <cstdlib>

int main(int argc, char* argv[]) {
  #if DATAGEN > 0
    std::cout << "Preprocessor Variable DATAGEN must be set to 0 for normal use";
    getchar();
    return EXIT_FAILURE;
  #endif
  search::init();

  if(argc > 1){
    if(std::string(argv[1]) == "bench"){
      uci::bench();
      return EXIT_SUCCESS;
    }
  }

  chess::Board board;
  std::cout << "Aurora " << VERSION_NUM VERSION_NAME DEV_STRING << ", a chess engine by kjljixx\n";
  uci::loop(board);
  return EXIT_SUCCESS;
}