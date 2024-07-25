#include "uci.h"
#include "datagen.h"

int main() {
  #ifdef DATAGEN
    datagen::main();
    return 1;
  #endif

  search::init();

  chess::Board board;
  std::cout << "Aurora " << VERSION_NUM VERSION_NAME DEV_STRING << ", a chess engine by kjljixx\n";
  uci::loop(board);
  return 1;
}