#include "uci.h"

int main() {
  std::string version = "0.0";
  lookupTables::init();
  std::cout << "Aurora " << version << ", a chess engine by kjljixx\n";
  uci::loop();
  return 1;
}