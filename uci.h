#pragma once
//Start reading code relating to move generation/chess rules in "bitboards.h"
//After reading this file, go to files relating to search, starting with "search.h"
#include "search.h"
#include <chrono>
//See https://backscattering.de/chess/uci/ for information on the Universal Chess Interface, which this file implements
namespace uci{

search::Node* root = nullptr;
chess::Board rootBoard;
search::Tree tree;

//bench stuff
const int AMOUNT_OF_FENS = 50;
const int SECONDS_PER_POSITION = 1;

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
			"r7/6k1/1p6/2pp1p2/7Q/8/p1P2K1P/8 w - - 0 32",
			"r3k2r/ppp1pp1p/2nqb1pn/3p4/4P3/2PP4/PP1NBPPP/R2QK1NR w KQkq - 1 5",
			"3r1rk1/1pp1pn1p/p1n1q1p1/3p4/Q3P3/2P5/PP1NBPPP/4RRK1 w - - 0 12",
			"5rk1/1pp1pn1p/p3Brp1/8/1n6/5N2/PP3PPP/2R2RK1 w - - 2 20",
			"8/1p2pk1p/p1p1r1p1/3n4/8/5R2/PP3PPP/4R1K1 b - - 3 27",
			"8/4pk2/1p1r2p1/p1p4p/Pn5P/3R4/1P3PP1/4RK2 w - - 1 33",
			"8/5k2/1pnrp1p1/p1p4p/P6P/4R1PK/1P3P2/4R3 b - - 1 38",
			"8/8/1p1kp1p1/p1pr1n1p/P6P/1R4P1/1P3PK1/1R6 b - - 15 45",
			"8/8/1p1k2p1/p1prp2p/P2n3P/6P1/1P1R1PK1/4R3 b - - 5 49",
			"8/8/1p4p1/p1p2k1p/P2npP1P/4K1P1/1P6/3R4 w - - 6 54",
			"8/8/1p4p1/p1p2k1p/P2n1P1P/4K1P1/1P6/6R1 b - - 6 59",
			"8/5k2/1p4p1/p1pK3p/P2n1P1P/6P1/1P6/4R3 b - - 14 63",
			"8/1R6/1p1K1kp1/p6p/P1p2P1P/6P1/1Pn5/8 w - - 0 67",
			"1rb1rn1k/p3q1bp/2p3p1/2p1p3/2P1P2N/PP1RQNP1/1B3P2/4R1K1 b - - 4 23",
			"4rrk1/pp1n1pp1/q5p1/P1pP4/2n3P1/7P/1P3PB1/R1BQ1RK1 w - - 3 22",
			"r2qr1k1/pb1nbppp/1pn1p3/2ppP3/3P4/2PB1NN1/PP3PPP/R1BQR1K1 w - - 4 12",
			"2r2k2/8/4P1R1/1p6/8/P4K1N/7b/2B5 b - - 0 55",
			"6k1/5pp1/8/2bKP2P/2P5/p4PNb/B7/8 b - - 1 44",
			"2rqr1k1/1p3p1p/p2p2p1/P1nPb3/2B1P3/5P2/1PQ2NPP/R1R4K w - - 3 25",
			"r1b2rk1/p1q1ppbp/6p1/2Q5/8/4BP2/PPP3PP/2KR1B1R b - - 2 14",
			"6r1/5k2/p1b1r2p/1pB1p1p1/1Pp3PP/2P1R1K1/2P2P2/3R4 w - - 1 36",
			"rnbqkb1r/pppppppp/5n2/8/2PP4/8/PP2PPPP/RNBQKBNR b KQkq c3 0 2",
			"2rr2k1/1p4bp/p1q1p1p1/4Pp1n/2PB4/1PN3P1/P3Q2P/2RR2K1 w - f6 0 20",
			"3br1k1/p1pn3p/1p3n2/5pNq/2P1p3/1PN3PP/P2Q1PB1/4R1K1 w - - 0 23",
			"2r2b2/5p2/5k2/p1r1pP2/P2pB3/1P3P2/K1P3R1/7R w - - 23 93"
	};

//The "position" command
chess::Board position(std::istringstream input){
  search::destroyTree(tree); root = nullptr;
  //tree.tt.init(search::TT_DEFAULT_SIZE);

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
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::ENPASSANT), rootBoard, root, tree);
    }
    //Castling
    else if((squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 2 || squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 6) && squareIndexToFile(squareNotationToIndex(token.substr(0, 2))) == 4 && board.findPiece(squareNotationToIndex(token.substr(0, 2))) == chess::KING){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::CASTLE), rootBoard, root, tree);
    }
    //Promotion
    else if(token.size() == 5){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::PROMOTION, chess::letterToPiece(token[4])), rootBoard, root, tree);
    }
    //Normal
    else{
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2))), rootBoard, root, tree);
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
    root = search::search(board, search::timeManagement(search::FOREVER), root, tree);
  }
  else if(token == "nodes"){
    int maxNodes;
    input >> maxNodes;
    root = search::search(board, search::timeManagement(search::NODES, maxNodes), root, tree);
  }
  else{
    search::timeManagement tm(search::TIME);
    int time;

    if(token == "wtime"){input >> time; if(board.sideToMove == chess::WHITE){tm.limit += std::max(Aurora::options["searchTimePortion"].value*time/1000.0, 0.005);}}
    else if(token == "btime"){input >> time; if(board.sideToMove == chess::BLACK){tm.limit += std::max(Aurora::options["searchTimePortion"].value*time/1000.0, 0.005);}}
    else if(token == "winc"){input >> time; if(board.sideToMove == chess::WHITE){tm.limit += 0/20000.0;}}
    else if(token == "binc"){input >> time; if(board.sideToMove == chess::BLACK){tm.limit += 0/20000.0;}}

    while(input >> token){
      if(token == "wtime"){input >> time; if(board.sideToMove == chess::WHITE){tm.limit += std::max(Aurora::options["searchTimePortion"].value*time/1000.0, 0.005);}}
      else if(token == "btime"){input >> time; if(board.sideToMove == chess::BLACK){tm.limit += std::max(Aurora::options["searchTimePortion"].value*time/1000.0, 0.005);}}
      else if(token == "winc"){input >> time; if(board.sideToMove == chess::WHITE){tm.limit += 0/20000.0;}}
      else if(token == "binc"){input >> time; if(board.sideToMove == chess::BLACK){tm.limit += 0/20000.0;}}
    }
    root = search::search(board, tm, root, tree);
  }
  rootBoard = board;
}
void respondUci(){
  std::cout <<  "id name Aurora " << VERSION << "\n"
                "id author kjljixx\n"
                "\n";
                for(auto option : Aurora::options){
                  std::cout << "option name " << option.first << " "
                                      "type " << (option.second.type == 0 ? "string" : "spin") << " "
                                      "default " << option.second.defaultValue << " "
                                      "min " << option.second.minValue << " "
                                      "max " << option.second.maxValue << "\n";
                }
                std::cout << "\nuciok\n";
}
void setOption(std::istringstream input){
  std::string token;

  input >> token; //input the "name" token

  std::string optionName;
  input >> optionName;

  if(Aurora::options.find(optionName) == Aurora::options.end()){
    std::cout << "info string could not find option " << optionName << "\n";
    return;
  }

  input >> token; //input the "value" token

  float optionValue;
  input >> optionValue;
  Aurora::options[optionName].value = optionValue;

  std::cout << "info string option " << optionName << " set to " << optionValue << "\n";
}
//Custom commands
chess::Board makeMoves(chess::Board &board, std::istringstream input){
  std::string token;
  while(input >> token){
    //En Passant
    if((1ULL << squareNotationToIndex(token.substr(2, 2))) & board.enPassant && board.findPiece(squareNotationToIndex(token.substr(0, 2))) == chess::PAWN){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::ENPASSANT), rootBoard, root, tree);
    }
    //Castling
    else if((squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 2 || squareIndexToFile(squareNotationToIndex(token.substr(2, 2))) == 6) && squareIndexToFile(squareNotationToIndex(token.substr(0, 2))) == 4 && board.findPiece(squareNotationToIndex(token.substr(0, 2))) == chess::KING){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::CASTLE), rootBoard, root, tree);
    }
    //Promotion
    else if(token.size() == 5){
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2)), chess::PROMOTION, chess::letterToPiece(token[4])), rootBoard, root, tree);
    }
    //Normal
    else{
      search::makeMove(board, chess::Move(uint8_t(squareNotationToIndex(token.substr(0, 2))), squareNotationToIndex(token.substr(2, 2))), rootBoard, root, tree);
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
  search::destroyTree(tree);

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
    if(token == "ucinewgame"){
      search::destroyTree(tree);
      root = nullptr;
      for(int i=0; i<(search::TT_DEFAULT_SIZE); i++){
        assert(tree.tt.tt[i].hash == 0);
      }
    }
    //non-uci, custom commands
    if(token == "moves"){std::getline(std::cin, token); board = makeMoves(board, std::istringstream(token));}
    //bwlow are mostly for debugging purposes
    if(token == "board"){board.printBoard(); std::cout << "\n";} 
    if(token == "checkmask"){bitboards::printBoard(board.generateKingMasks().checkmask); std::cout << "\n";}
    if(token == "rpinmask"){bitboards::printBoard(board.generateKingMasks().rookPinmask); std::cout << "\n";}
    if(token == "rpinned"){bitboards::printBoard(board.generateKingMasks().rookPinnedPieces); std::cout << "\n";}
    if(token == "bpinmask"){bitboards::printBoard(board.generateKingMasks().bishopPinmask); std::cout << "\n";}
    if(token == "bpinned"){bitboards::printBoard(board.generateKingMasks().bishopPinnedPieces); std::cout << "\n";}
    if(token == "staticeval"){evaluation::NNUE nnue; nnue.refreshAccumulator(board); std::cout << evaluation::evaluate(board, nnue) << "\n";}
    if(token == "see"){std::cin >> token; uint8_t square = squareNotationToIndex(token); std::cout << evaluation::SEE(board, square, 0) << "\n";}
    if(token == "zobrist"){std::cout << zobrist::getHash(board) << "\n";}
    if(token == "bench"){
      int i = 0;
      int nodes = 0;

      int currOutputLevel = Aurora::options["outputLevel"].value;
      Aurora::options["outputLevel"].value = 0;

      for(std::string fen : benchFens){
        i++;
        chess::Board board(fen);

        search::Node* root = search::search(board, search::timeManagement(search::TIME, SECONDS_PER_POSITION), nullptr, tree);

        nodes += root->visits;

        search::destroyTree(tree); root = nullptr;
        std::cout << "\nFINISHED POSITION #" << i << " OF 50";
      }
      std::cout << "\nnps: " << double(nodes)/(SECONDS_PER_POSITION*i);

      Aurora::options["outputLevel"].value = currOutputLevel;
    }
  }
}
}