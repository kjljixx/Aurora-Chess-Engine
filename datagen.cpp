#include "search.h"
#include <fstream>
#include <iomanip>
#include <thread>

int openingLength = 8;

int infoPrintInterval = 10;

int numberOfThreads = 8;

int main(){
  #if DATAGEN == 0
    std::cout << "Preprocessor Variable DATAGEN must be set to 1 or 2 to Generate Data";
    getchar();
    return 0;
  #else

  std::string version = VERSION;
  version += "-datagen";

  search::init();
  //tb_init("C:\\Users\\kjlji\\OneDrive\\Documents\\VSCode\\C++\\AuroraChessEngine-main\\3-4-5");

  std::cout << "Aurora " << version << ", a chess engine by kjljixx\n";

  std::vector<std::thread> threads;
  threads.reserve(numberOfThreads);
  for(int threadId=1; threadId<=numberOfThreads; threadId++) {
    threads.emplace_back([threadId, version] {
      std::random_device rd; 
      std::mt19937 eng(rd());

      std::vector<std::string> gameData;

      search::timeManagement tm(search::NODES, 360);

      chess::Board board;
      chess::Board rootBoard; //Only exists to make the search::makeMove function happy
      search::Tree tree;

      bool validOpening = false;
      while(validOpening == false){
        board.setToFen(chess::startPosFen);

        for(int i=0; i<openingLength; i++){
          chess::MoveList moves(board);

          if(moves.size()==0){break;}

          std::uniform_int_distribution<> distr(0, moves.size() - 1);
          int moveIndex = distr(eng);
          chess::makeMove(board, moves[moveIndex]);
        }
        if(chess::getGameStatus(board, chess::isLegalMoves(board)) == chess::ONGOING){
          validOpening = true;
        }
      }
      rootBoard = board;

      int gameIter = 0;
      int fenIter = 0;

      int previousFenIter = 0;
      int previousElapsed = 0;

      auto start = std::chrono::steady_clock::now();

      search::Node* root = nullptr;

      while(true){
        if(chess::getGameStatus(board, chess::isLegalMoves(board)) != chess::ONGOING){
          for(std::string currData : gameData){
            std::cout << "\n" << currData;
          }
          std::cout << "\n";
          board.printBoard();
          std::cout << "\n" << chess::getGameStatus(board, chess::isLegalMoves(board)) << " " << root->isTerminal;
          assert(0);
        }

        root = search::search(board, tm, root, tree);

        if(board.squareUnderAttack(_bitscanForward(board.getOurPieces(chess::KING)))==64 && board.mailbox[0][search::findBestEdge(root).move.getEndSquare()]==0 && std::abs(root->value)<0.9999){
          gameData.push_back(board.getFen() + " | " + std::to_string(int(round(tan((board.sideToMove ? search::findBestValue(root) : -search::findBestValue(root))*1.56375)*100))));
          fenIter++;
        }

        search::makeMove(board, search::findBestEdge(root).move, rootBoard, root, tree);

        if(root->isTerminal || std::abs(root->value)>0.9999){
          gameIter++;
          if(gameIter % infoPrintInterval == 0){
            std::cout << "Thread: " << threadId << "\n";
            std::cout << "Games: " << gameIter << "\n";
            std::cout << "FENs: " << fenIter << "\n";
            std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;
            std::cout << "Overall FENs/second: " << fenIter / elapsed.count() << "\n";
            std::cout << "Recent FENs/second: " << (fenIter - previousFenIter) / (elapsed.count() - previousElapsed) << "\n";

            previousElapsed = elapsed.count();
            previousFenIter = fenIter;
          }

          std::stringstream stream;
          stream << std::fixed << std::setprecision(1) << ((board.sideToMove ? -root->value : root->value) + 1) / 2.0;
          std::string gameResultStr = stream.str();

          std::ofstream dataFile;
          dataFile.open(dataFolderPath+"/"+version+"-thread"+std::to_string(threadId)+".txt", std::ios_base::app);
          for(std::string currData : gameData){
            currData += " | " + gameResultStr + "\n";
            
            dataFile << currData;
          }
          gameData.clear();
          dataFile.close();

          search::destroyTree(tree);
          root = nullptr;

          validOpening = false;
          while(validOpening == false){
            board.setToFen(chess::startPosFen);

            for(int i=0; i<openingLength; i++){
              chess::MoveList moves(board);

              if(moves.size()==0){break;}
              std::uniform_int_distribution<> distr(0, moves.size() - 1);
              int moveIndex = distr(eng);
              chess::makeMove(board, moves[moveIndex]);
            }
            if(chess::getGameStatus(board, chess::isLegalMoves(board)) == chess::ONGOING){
              validOpening = true;
            }
            else{}
          }
          rootBoard = board;
        }
      }
    });
  }

  for(auto& thread : threads){
    thread.join();
  }
  #endif
  return 1;
}