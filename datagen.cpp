#include "search.h"
#include <fstream>

int main() {
  #if DATAGEN >= 1
    std::cout << "Preprocessor Variable DATAGEN must be set to 1 to Generate Data";
    getchar();
    return 0;
  #endif
  std::string version = "0.10.1-tunedEvaluation";
  #if DATAGEN == 1
    version += "-datagen";
  #endif
  search::init();
  //tb_init("C:\\Users\\kjlji\\OneDrive\\Documents\\VSCode\\C++\\AuroraChessEngine-main\\3-4-5");

  std::cout << "Aurora " << version << ", a chess engine by kjljixx\n";

  std::ifstream book;
  book.open("C:/Users/kjlji/Downloads/UHO_4060_v2.epd/UHO_4060_v2.epd");

  std::string fen;
  std::getline(book, fen);
  chess::Board board(fen);

  search::timeManagement tm(search::NODES, 50000);
  while(true){
    search::search(board, tm);
    search::makeMove(board, search::findBestChild(search::root)->edge);
    if(search::root->isTerminal){
      search::destroyTree(search::root);
      search::root = nullptr;
      std::getline(book, fen);
      board.setToFen(fen);
    }
  }
  return 1;
}