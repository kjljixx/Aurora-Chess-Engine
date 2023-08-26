//Start reading code relating to move generation/chess rules in "bitboards.h"
//After reading this file, go to files relating to search, starting with "search.h"
#include "search.h"
#include <chrono>
//See https://backscattering.de/chess/uci/ for information on the Universal Chess Interface, which this file implements
namespace uci{
//The "position" command
chess::Board position(std::istringstream input){
  std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  std::string token;

  input >> token;

  if(token == "fen"){
    input >> fen;
    while(input >> token && token != "moves"){
      fen += " " + token;
    }
  }
  else{
    //input the moves token, if there is one
    input >> token;
  }

  chess::Board board(fen);

  while(input >> token){
    //En Passant
    if((1ULL << squareNotationToIndex(token.substr(2, 2))) & board.enPassant){
      board.makeMove(chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::ENPASSANT));
    }
    //Castling
    else if((squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 2 || squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 6) && squareIndexToFile(squareNotationToIndex(token.substr(0, 2))) == 4 && board.findPiece(squareNotationToIndex(token.substr(0, 2))) == chess::KING){
      board.makeMove(chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::CASTLE));
    }
    //Promotion
    else if(token.size() == 5){
      board.makeMove(chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::PROMOTION, chess::letterToPiece(token[4])));
    }
    //Normal
    else{
      board.makeMove(chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2))));
    }
  }

  return board;
}
//The "perft" command
uint64_t perft(chess::Board &board, int depth, bool printResults){
  uint64_t nodes = 0;

  if(depth == 1 && !printResults){
    return chess::MoveList(board).size();
  }

  if(depth == 0){
    return 1;
  }

  for(chess::Move move : chess::MoveList(board)){
    chess::Board movedBoard = board;
    movedBoard.makeMove(move);

    uint64_t result = perft(movedBoard, depth-1, false);
    if(printResults){std::cout << move.toStringRep() << ": " << result << "\n";}
    nodes+=result;
  }

  return nodes;
}
uint64_t perftDiv(chess::Board &board, int depth){
  chess::Board movedBoard;
  uint64_t nodes = 0;

  auto start = std::chrono::steady_clock::now();

  nodes = perft(board, depth, true);

  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed = end - start;

  std::cout << std::endl << "==========================" << std::endl;
  std::cout << "Total time (ms) : " << static_cast<int>(elapsed.count() * 1000) << std::endl;
  std::cout << "Nodes searched  : " << nodes << std::endl;
  std::cout << "Nodes / second  : " << static_cast<int>(nodes / elapsed.count()) << std::endl;

  return nodes;
}
//Custom commands
chess::Board makeMoves(chess::Board &board, std::istringstream input){
  std::string token;
  while(input >> token){
    //En Passant
    if((1ULL << squareNotationToIndex(token.substr(2, 2))) & board.enPassant){
      board.makeMove(chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::ENPASSANT));
    }
    //Castling
    else if((squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 2 || squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 6) && squareIndexToFile(squareNotationToIndex(token.substr(0, 2))) == 4 && board.findPiece(squareNotationToIndex(token.substr(0, 2))) == chess::KING){
      board.makeMove(chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::CASTLE));
    }
    //Promotion
    else if(token.size() == 5){
      board.makeMove(chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::PROMOTION, chess::letterToPiece(token[4])));
    }
    //Normal
    board.makeMove(chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2))));
  }
  return board;
}

void go(std::istringstream input, chess::Board board){
  std::string token;

  input >> token;
  if(token == "infinite"){
    search::search(board, uint32_t(-1));
  }
  if(token == "nodes"){
    uint32_t maxNodes;
    input >> maxNodes;
    search::search(board, maxNodes);
  }
}

//The main UCI loop which detects input and runs other functions based on it
void loop(chess::Board board){
  std::string token;

  while(true){
    std::cin >> token;
    if(token == "uci"){std::cout << "id name Aurora\nid author kjljixx\n\nuciok\n";} //TODO: add engine options before printing uciok
    if(token == "isready"){std::cout << "readyok\n";} //TODO: make sure we are actually ready before printing readyok
    if(token == "perft"){int depth; std::cin >> depth; perftDiv(board, depth);}
    if(token == "position"){std::getline(std::cin, token); board = position(std::istringstream(token));}
    if(token == "go"){std::getline(std::cin, token); go(std::istringstream(token), board);}
    if(token == "quit"){break;}
    //non-uci, custom commands
    if(token == "moves"){std::getline(std::cin, token); board = makeMoves(board, std::istringstream(token));}
    //bwlow are mostly for debugging purposes
    if(token == "board"){board.printBoard(); std::cout << "\n";} 
    if(token == "checkmask"){bitboards::printBoard(board.generateKingMasks().checkmask); std::cout << "\n";}
    if(token == "rpinmask"){bitboards::printBoard(board.generateKingMasks().rookPinmask); std::cout << "\n";}
    if(token == "rpinned"){bitboards::printBoard(board.generateKingMasks().rookPinnedPieces); std::cout << "\n";}
    if(token == "bpinmask"){bitboards::printBoard(board.generateKingMasks().bishopPinmask); std::cout << "\n";}
    if(token == "bpinned"){bitboards::printBoard(board.generateKingMasks().bishopPinnedPieces); std::cout << "\n";}
    if(token == "staticeval"){std::cout << evaluation::evaluate(board) << "\n";}
  }
}
}