#include "framework.h"
#include "lookup.h"
#include "zobrist.h"

int main() {
  lookupTables::init();
  zobrist::init();
  return aurora_test::run_all();
}
