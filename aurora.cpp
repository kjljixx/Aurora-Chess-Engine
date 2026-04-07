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
      uci::bench();
      return 1;
    }
  }

  chess::Board board;
  std::cout << "Aurora " << VERSION_NUM VERSION_NAME DEV_STRING << ", a chess engine by kjljixx\n";
  uci::loop(board);
  return 1;
}