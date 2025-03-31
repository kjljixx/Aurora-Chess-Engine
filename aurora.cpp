#include "datagen.h"
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
    else if(std::string(argv[1]).starts_with("genfens")){
      std::istringstream stream(argv[1]);
      std::string token;
      
      int numFens;
      stream >> token;
      stream >> numFens;
      
      uint64_t seed;
      stream >> token;
      stream >> seed;

      std::string bookFileName;
      stream >> token;
      stream >> bookFileName;

      int depth;
      stream >> depth;

      std::vector<std::string> book;
      if(bookFileName != "None"){
        std::ifstream bookFile(bookFileName);
        while(std::getline(bookFile, token)){
          book.push_back(token);
        }
        datagen::genFens(numFens, seed, depth, book);
      }
      else{
        datagen::genFens(numFens, seed, depth);
      }
      
      return 1;
    }
  }

  chess::Board board;
  std::cout << "Aurora " << VERSION_NUM VERSION_NAME DEV_STRING << ", a chess engine by kjljixx\n";
  uci::loop(board);
  return 1;
}