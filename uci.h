#pragma once
//Start reading code relating to move generation/chess rules in "bitboards.h"
//After reading this file, go to files relating to search, starting with "search.h"
#include "search.h"
#include <chrono>
//See https://backscattering.de/chess/uci/ for information on the Universal Chess Interface, which this file implements
namespace uci{

//Some Parameters
float searchTimeFactor = 0.05;

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
    if((1ULL << squareNotationToIndex(token.substr(2, 2))) & board.enPassant && board.findPiece(squareNotationToIndex(token.substr(0, 2))) == chess::PAWN){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::ENPASSANT));
    }
    //Castling
    else if((squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 2 || squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 6) && squareIndexToFile(squareNotationToIndex(token.substr(0, 2))) == 4 && board.findPiece(squareNotationToIndex(token.substr(0, 2))) == chess::KING){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::CASTLE));
    }
    //Promotion
    else if(token.size() == 5){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::PROMOTION, chess::letterToPiece(token[4])));
    }
    //Normal
    else{
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2))));
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
void go(std::istringstream input, chess::Board board){
  std::string token;

  input >> token;
  if(token == "infinite"){
    search::search(board, search::timeManagement(search::INFINITE));
  }
  else if(token == "nodes"){
    int maxNodes;
    input >> maxNodes;
    search::search(board, search::timeManagement(search::NODES, maxNodes));
  }
  else{
    search::timeManagement tm(search::TIME);
    int time;

    if(token == "wtime"){input >> time; if(board.sideToMove == chess::WHITE){tm.limit += std::max(searchTimeFactor*time/1000.0, 0.005);}}
    else if(token == "btime"){input >> time; if(board.sideToMove == chess::BLACK){tm.limit += std::max(searchTimeFactor*time/1000.0, 0.005);}}
    else if(token == "winc"){input >> time; if(board.sideToMove == chess::WHITE){tm.limit += 0/20000.0;}}
    else if(token == "binc"){input >> time; if(board.sideToMove == chess::BLACK){tm.limit += 0/20000.0;}}

    while(input >> token){
      if(token == "wtime"){input >> time; if(board.sideToMove == chess::WHITE){tm.limit += std::max(searchTimeFactor*time/1000.0, 0.005);}}
      else if(token == "btime"){input >> time; if(board.sideToMove == chess::BLACK){tm.limit += std::max(searchTimeFactor*time/1000.0, 0.005);}}
      else if(token == "winc"){input >> time; if(board.sideToMove == chess::WHITE){tm.limit += 0/20000.0;}}
      else if(token == "binc"){input >> time; if(board.sideToMove == chess::BLACK){tm.limit += 0/20000.0;}}
    }
    search::search(board, tm);
  }
}
void respondUci(){
  std::cout <<  "id name Aurora\n"
                "id author kjljixx\n"
                "\n"
                "option name outputlevel type spin default " << search::outputLevel << " min 0 max 3\n"
                "option name explorationfactor type string default " << search::explorationFactor << "\n"
                "option name evalscalefactor type string default " << search::evalScaleFactor << "\n"
                "option name evalstabilitybias type spin default " << evaluation::evalStabilityConstant << " min -1024 max 1024\n"
                "option name searchtimeportion type string default " << searchTimeFactor << "\n"
                "option name mg_passedpawnbonus2 type spin default " << evaluation::mg_passedPawnBonus[1] << " min -1024 max 1024\n"
                "option name mg_passedpawnbonus3 type spin default " << evaluation::mg_passedPawnBonus[2] << " min -1024 max 1024\n"
                "option name mg_passedpawnbonus4 type spin default " << evaluation::mg_passedPawnBonus[3] << " min -1024 max 1024\n"
                "option name mg_passedpawnbonus5 type spin default " << evaluation::mg_passedPawnBonus[4] << " min -1024 max 1024\n"
                "option name mg_passedpawnbonus6 type spin default " << evaluation::mg_passedPawnBonus[5] << " min -1024 max 1024\n"
                "option name mg_passedpawnbonus7 type spin default " << evaluation::mg_passedPawnBonus[6] << " min -1024 max 1024\n"
                "option name eg_passedpawnbonus2 type spin default " << evaluation::eg_passedPawnBonus[1] << " min -1024 max 1024\n"
                "option name eg_passedpawnbonus3 type spin default " << evaluation::eg_passedPawnBonus[2] << " min -1024 max 1024\n"
                "option name eg_passedpawnbonus4 type spin default " << evaluation::eg_passedPawnBonus[3] << " min -1024 max 1024\n"
                "option name eg_passedpawnbonus5 type spin default " << evaluation::eg_passedPawnBonus[4] << " min -1024 max 1024\n"
                "option name eg_passedpawnbonus6 type spin default " << evaluation::eg_passedPawnBonus[5] << " min -1024 max 1024\n"
                "option name eg_passedpawnbonus7 type spin default " << evaluation::eg_passedPawnBonus[6] << " min -1024 max 1024\n"
                "\n"
                "uciok\n";
}
void setOption(std::istringstream input){
  std::string token;
  input >> token; //input the "name" token
  input >> token;
  if(token == "outputlevel"){
    input >> token; //input the "value" token
    int value;
    input >> value;
    search::outputLevel = value;
  }
  if(token == "explorationfactor"){
    input >> token; //input the "value" token
    float value;
    input >> value;
    search::explorationFactor = value;
  }
  if(token == "evalstabilitybias"){
    input >> token; //input the "value" token
    int value;
    input >> value;
    evaluation::evalStabilityConstant = value;
  }
  if(token == "evalscalefactor"){
    input >> token; //input the "value" token
    float value;
    input >> value;
    search::evalScaleFactor = value;
  }
  if(token == "searchtimeportion"){
    input >> token; //input the "value" token
    float value;
    input >> value;
    searchTimeFactor = value;
  }
  std::string prefixes[2] = {"mg_", "eg_"};
  std::string postfixes[6] = {"2", "3", "4", "5", "6", "7"};
  for(int i=0; i<2; i++){
    for(int j=0; j<6; j++){
      if(token == prefixes[i]+"passedpawnbonus"+postfixes[j]){
          input >> token; //input the "value" token
          int value;
          input >> value;
          evaluation::passedPawnBonuses[i][j+1] = value;
      }
    }
  }
}
//Custom commands
chess::Board makeMoves(chess::Board &board, std::istringstream input){
  std::string token;
  while(input >> token){
    //En Passant
    if((1ULL << squareNotationToIndex(token.substr(2, 2))) & board.enPassant && board.findPiece(squareNotationToIndex(token.substr(0, 2))) == chess::PAWN){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::ENPASSANT));
    }
    //Castling
    else if((squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 2 || squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 6) && squareIndexToFile(squareNotationToIndex(token.substr(0, 2))) == 4 && board.findPiece(squareNotationToIndex(token.substr(0, 2))) == chess::KING){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::CASTLE));
    }
    //Promotion
    else if(token.size() == 5){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::PROMOTION, chess::letterToPiece(token[4])));
    }
    //Normal
    else{
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2))));
    }
  }
  return board;
}
chess::Move getMoveFromString(chess::Board &board, std::string token){
  chess::Move move;
  //En Passant
  if((1ULL << squareNotationToIndex(token.substr(2, 2))) & board.enPassant && board.findPiece(squareNotationToIndex(token.substr(0, 2))) == chess::PAWN){
    move = chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::ENPASSANT);
  }
  //Castling
  else if((squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 2 || squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 6) && squareIndexToFile(squareNotationToIndex(token.substr(0, 2))) == 4 && board.findPiece(squareNotationToIndex(token.substr(0, 2))) == chess::KING){
    move = chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::CASTLE);
  }
  //Promotion
  else if(token.size() == 5){
    move = chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::PROMOTION, chess::letterToPiece(token[4]));
  }
  //Normal
  else{
    move = chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)));
  }
  return move;
}


//The main UCI loop which detects input and runs other functions based on it
void loop(chess::Board board){

  std::string token;

  while(true){
    std::cin >> token;
    if(token == "uci"){respondUci();}
    if(token == "setoption"){std::getline(std::cin, token); setOption(std::istringstream(token));}
    if(token == "isready"){std::cout << "readyok\n";} //TODO: make sure we are actually ready before printing readyok
    if(token == "perft"){int depth; std::cin >> depth; perftDiv(board, depth);}
    if(token == "position"){std::getline(std::cin, token); board = position(std::istringstream(token));}
    if(token == "go"){std::getline(std::cin, token); go(std::istringstream(token), board);}
    if(token == "quit"){break;}
    if(token == "ucinewgame"){search::destroyTree(search::root); search::root = nullptr;}
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
    if(token == "see"){std::cin >> token; uint8_t square = squareNotationToIndex(token); std::cout << evaluation::SEE(board, square) << "\n";}
    if(token == "zobrist"){std::cout << zobrist::getHash(board) << "\n";}
  }
}
}