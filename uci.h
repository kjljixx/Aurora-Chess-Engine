#pragma once
//Start reading code relating to move generation/chess rules in "bitboards.h"
//After reading this file, go to files relating to search, starting with "search.h"
#include "search.h"
#include <chrono>
//See https://backscattering.de/chess/uci/ for information on the Universal Chess Interface, which this file implements
namespace uci{

search::Node* root;
chess::Board rootBoard;
search::Tree tree;

//bench stuff
const int AMOUNT_OF_FENS = 25;

std::string benchFens[AMOUNT_OF_FENS] = { //From Alexandria
			"r3k2r/2pb1ppp/2pp1q2/p7/1nP1B3/1P2P3/P2N1PPP/R2QK2R w KQkq a6 0 14",
			"4rrk1/2p1b1p1/p1p3q1/4p3/2P2n1p/1P1NR2P/PB3PP1/3R1QK1 b - - 2 24",
			"r3qbrk/6p1/2b2pPp/p3pP1Q/PpPpP2P/3P1B2/2PB3K/R5R1 w - - 16 42",
			"6k1/1R3p2/6p1/2Bp3p/3P2q1/P7/1P2rQ1K/5R2 b - - 4 44",
			"8/8/1p2k1p1/3p3p/1p1P1P1P/1P2PK2/8/8 w - - 3 54",
			"7r/2p3k1/1p1p1qp1/1P1Bp3/p1P2r1P/P7/4R3/Q4RK1 w - - 0 36",
			"r1bq1rk1/pp2b1pp/n1pp1n2/3P1p2/2P1p3/2N1P2N/PP2BPPP/R1BQ1RK1 b - - 2 10",
			"3r3k/2r4p/1p1b3q/p4P2/P2Pp3/1B2P3/3BQ1RP/6K1 w - - 3 87",
			"2r4r/1p4k1/1Pnp4/3Qb1pq/8/4BpPp/5P2/2RR1BK1 w - - 0 42",
			"4q1bk/6b1/7p/p1p4p/PNPpP2P/KN4P1/3Q4/4R3 b - - 0 37",
			"2q3r1/1r2pk2/pp3pp1/2pP3p/P1Pb1BbP/1P4Q1/R3NPP1/4R1K1 w - - 2 34",
			"1r2r2k/1b4q1/pp5p/2pPp1p1/P3Pn2/1P1B1Q1P/2R3P1/4BR1K b - - 1 37",
			"r3kbbr/pp1n1p1P/3ppnp1/q5N1/1P1pP3/P1N1B3/2P1QP2/R3KB1R b KQkq b3 0 17",
			"8/6pk/2b1Rp2/3r4/1R1B2PP/P5K1/8/2r5 b - - 16 42",
			"1r4k1/4ppb1/2n1b1qp/pB4p1/1n1BP1P1/7P/2PNQPK1/3RN3 w - - 8 29",
			"8/p2B4/PkP5/4p1pK/4Pb1p/5P2/8/8 w - - 29 68",
			"3r4/ppq1ppkp/4bnp1/2pN4/2P1P3/1P4P1/PQ3PBP/R4K2 b - - 2 20",
			"5rr1/4n2k/4q2P/P1P2n2/3B1p2/4pP2/2N1P3/1RR1K2Q w - - 1 49",
			"1r5k/2pq2p1/3p3p/p1pP4/4QP2/PP1R3P/6PK/8 w - - 1 51",
			"q5k1/5ppp/1r3bn1/1B6/P1N2P2/BQ2P1P1/5K1P/8 b - - 2 34",
			"r1b2k1r/5n2/p4q2/1ppn1Pp1/3pp1p1/NP2P3/P1PPBK2/1RQN2R1 w - - 0 22",
			"r1bqk2r/pppp1ppp/5n2/4b3/4P3/P1N5/1PP2PPP/R1BQKB1R w KQkq - 0 5",
			"r1bqr1k1/pp1p1ppp/2p5/8/3N1Q2/P2BB3/1PP2PPP/R3K2n b Q - 1 12",
			"r1bq2k1/p4r1p/1pp2pp1/3p4/1P1B3Q/P2B1N2/2P3PP/4R1K1 b - - 2 19",
			"r4qk1/6r1/1p4p1/2ppBbN1/1p5Q/P7/2P3PP/5RK1 w - - 2 25",
};

void bench(){
  int nodes = 0;
  Aurora::options["outputLevel"].value = 0;

  float totalElapsed = 0;

  for(std::string fen : benchFens){
    chess::Board board(fen);

    auto start = std::chrono::steady_clock::now();

    search::search(board, search::timeManagement(search::ITERS, 10000), tree);

    std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;
    totalElapsed += elapsed.count();

    search::Node* root = tree.root;

    nodes += root->visits;

    search::destroyTree(tree); root = nullptr;
  }


  std::cout << "\n" << nodes << " nodes " << int(nodes/totalElapsed) << " nps" << std::endl;
}

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
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::ENPASSANT), rootBoard, tree);
    }
    //Castling
    else if((squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 2 || squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 6) && squareIndexToFile(squareNotationToIndex(token.substr(0, 2))) == 4 && board.findPiece(squareNotationToIndex(token.substr(0, 2))) == chess::KING){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::CASTLE), rootBoard, tree);
    }
    //Promotion
    else if(token.size() == 5){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::PROMOTION, chess::letterToPiece(token[4])), rootBoard, tree);
    }
    //Normal
    else{
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2))), rootBoard, tree);
    }
  }

  std::cout << "info string position set to " << board.getFen() << std::endl;

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
    if(zobrist::getHash(board) != zobrist::getHash(rootBoard)){search::destroyTree(tree);}
    search::search(board, search::timeManagement(search::FOREVER), tree);
  }
  else if(token == "nodes"){
    int maxNodes;
    input >> maxNodes;
    if(zobrist::getHash(board) != zobrist::getHash(rootBoard)){search::destroyTree(tree);}
    search::search(board, search::timeManagement(search::NODES, maxNodes), tree);
  }
  else if(token == "iters"){
    int maxIters;
    input >> maxIters;
    if(zobrist::getHash(board) != zobrist::getHash(rootBoard)){search::destroyTree(tree);}
    search::search(board, search::timeManagement(search::ITERS, maxIters), tree);
  }
  else{
    search::timeManagement tm(search::TIME);
    int time;

    int ourTime = 0;
    int ourInc = 0;
    int theirTime = 0;
    int theirInc = 0;

    if(token == "wtime"){input >> time; if(board.sideToMove == chess::WHITE){ourTime = time;}else{theirTime = time;}}
    else if(token == "btime"){input >> time; if(board.sideToMove == chess::BLACK){ourTime = time;}else{theirTime = time;}}
    else if(token == "winc"){input >> time; if(board.sideToMove == chess::WHITE){ourInc = time;}else{theirInc = time;}}
    else if(token == "binc"){input >> time; if(board.sideToMove == chess::BLACK){ourInc = time;}else{theirInc = time;}}

    while(input >> token){
      if(token == "wtime"){input >> time; if(board.sideToMove == chess::WHITE){ourTime = time;}else{theirTime = time;}}
      else if(token == "btime"){input >> time; if(board.sideToMove == chess::BLACK){ourTime = time;}else{theirTime = time;}}
      else if(token == "winc"){input >> time; if(board.sideToMove == chess::WHITE){ourInc = time;}else{theirInc = time;}}
      else if(token == "binc"){input >> time; if(board.sideToMove == chess::BLACK){ourInc = time;}else{theirInc = time;}}
    }
    int movesLeft = 30;
    int allocatedTime = fminf(0.05*(ourTime + ourInc*movesLeft), fmaxf(ourTime-50, 1));
    tm.limit = allocatedTime/1000.0;
    allocatedTime = fminf(0.1*(ourTime + ourInc*movesLeft), fmaxf(ourTime-50, 1));
    tm.hardLimit = allocatedTime/1000.0;
    if(zobrist::getHash(board) != zobrist::getHash(rootBoard)){search::destroyTree(tree);}
    search::search(board, tm, tree);
  }
  rootBoard = board;
  root = tree.root;
}
void respondUci(){
  std::cout <<  "id name Aurora " << VERSION_NUM DEV_STRING << "\n"
                "id author kjljixx\n"
                "\n";
                for(auto option : Aurora::options){
                  if(option.second.type == 2){
                    std::cout << "option name " << option.first << " "
                                        "type " << "string" << " "
                                        "default " << option.second.sDefaultValue << "\n";                  
                  }
                  else{
                    std::cout << "option name " << option.first << " "
                                        "type " << (option.second.type == 1 ? "spin" : "string") << " "
                                        "default " << option.second.defaultValue << " "
                                        "min " << option.second.minValue << " "
                                        "max " << option.second.maxValue << "\n";
                  }
                }
                std::cout << "\nuciok" << std::endl;
}
void setOption(std::istringstream input){
  std::string token;

  input >> token; //input the "name" token

  std::string optionName;
  input >> optionName;

  if(Aurora::options.find(optionName) == Aurora::options.end()){
    std::cout << "info string could not find option " << optionName << std::endl;
    return;
  }

  input >> token; //input the "value" token

  if (Aurora::options[optionName].type == 2){
    std::string optionValue;
    input >> optionValue;
    Aurora::options[optionName].sValue = optionValue;
    if(optionName != "SyzygyPath" || (tb_init(Aurora::options[optionName].sValue.c_str()) && TB_LARGEST > 0)){
      std::cout << "info string option " << optionName << " set to " << optionValue << std::endl;
    }
    else{
      std::cout << "info string could not init syzygy tablebases" << std::endl;
    }
  }
  else{
    float optionValue;
    input >> optionValue;
    Aurora::options[optionName].value = optionValue;
    std::cout << "info string option " << optionName << " set to " << optionValue << std::endl;
  }

}
//Custom commands
chess::Board makeMoves(chess::Board &board, std::istringstream input){
  std::string token;
  while(input >> token){
    //En Passant
    if((1ULL << squareNotationToIndex(token.substr(2, 2))) & board.enPassant && board.findPiece(squareNotationToIndex(token.substr(0, 2))) == chess::PAWN){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::ENPASSANT), rootBoard, tree);
    }
    //Castling
    else if((squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 2 || squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 6) && squareIndexToFile(squareNotationToIndex(token.substr(0, 2))) == 4 && board.findPiece(squareNotationToIndex(token.substr(0, 2))) == chess::KING){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::CASTLE), rootBoard, tree);
    }
    //Promotion
    else if(token.size() == 5){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::PROMOTION, chess::letterToPiece(token[4])), rootBoard, tree);
    }
    //Normal
    else{
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2))), rootBoard, tree);
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
    if(token == "build"){std::cout << GIT_HASH_STRING << std::endl;}
    if(token == "setoption"){std::getline(std::cin, token); setOption(std::istringstream(token));}
    if(token == "isready"){std::cout << "readyok" << std::endl;} //TODO: make sure we are actually ready before printing readyok
    if(token == "perft"){int depth; std::cin >> depth; perftDiv(board, depth);}
    if(token == "position"){std::getline(std::cin, token); board = position(std::istringstream(token));}
    if(token == "go"){std::getline(std::cin, token); go(std::istringstream(token), board);}
    if(token == "quit"){break;}
    if(token == "ucinewgame"){search::destroyTree(tree); root = nullptr; std::cout << "info string search tree destroyed" << std::endl;}
    //non-uci, custom commands
    if(token == "moves"){std::getline(std::cin, token); board = makeMoves(board, std::istringstream(token));}
    //bwlow are mostly for debugging purposes
    if(token == "debug"){setOption(std::istringstream("name outputLevel value 3"));}
    if(token == "fen"){std::getline(std::cin, token);board = position(std::istringstream("fen " + token));}

    if(token == "board"){board.printBoard(); std::cout << std::endl;}

    if(token == "checkmask"){bitboards::printBoard(board.generateKingMasks().checkmask); std::cout << std::endl;}
    if(token == "rpinmask"){bitboards::printBoard(board.generateKingMasks().rookPinmask); std::cout << std::endl;}
    if(token == "rpinned"){bitboards::printBoard(board.generateKingMasks().rookPinnedPieces); std::cout << std::endl;}
    if(token == "bpinmask"){bitboards::printBoard(board.generateKingMasks().bishopPinmask); std::cout << std::endl;}
    if(token == "bpinned"){bitboards::printBoard(board.generateKingMasks().bishopPinnedPieces); std::cout << std::endl;}
    
    if(token == "staticeval"){evaluation::NNUE<NNUEhiddenNeurons> nnue(evaluation::_NNUEparameters); nnue.refreshAccumulator(board); std::cout << evaluation::evaluate(board, nnue) << std::endl;}
    if(token == "see"){std::cin >> token; uint8_t square = squareNotationToIndex(token); std::cout << evaluation::SEE(board, square, 0) << std::endl;}
    
    if(token == "zobrist"){std::cout << zobrist::getHash(board) << std::endl;}
    if(token == "bench"){bench();}
  }
}
}