/*#include "src\position.h"
#include "src\tables.h"
#include "src\types.h"
#include "src\position.cpp"
#include "src\tables.cpp"
#include "src\types.cpp"*/
#include <vector>
#include <iostream>
#include "board.h"
#include <chrono>

/*template<Color Us>
void search(Position& p){
    MoveList<Us> list(p);
	//if (depth == 1) return (unsigned long long) list.size();
    int moveNumber = rand() % list.size();
    int it = 0;
	for (Move move : list) {
        if(it==moveNumber){
            p.play<Us>(move);
            std::cout <;
            search<~Us>(p);
        }
        it++;
	}
}
*/
int perftTest(chess::board &currentBoard, int depth, int maxDepth){ //maxDepth for debug/move breakdown only
    int nodes = 0;
    if(depth==maxDepth){
        currentBoard.outputMoves = true;
    }
    else{
        currentBoard.outputMoves = false;
    }
    std::vector<chess::board> legalMoves = chess::generateMoves(currentBoard);
    if(depth==1){return legalMoves.size();}
    for(int i=0; i<legalMoves.size(); i++){
        int result = perftTest(legalMoves[i], depth-1, maxDepth);
        if(depth==maxDepth){
            std::cout << result << "\n";
        }
        nodes += result;
    }
    return nodes;
}


int main() {
    int n = 0;
    chess::board test;
    std::string h = "8/3N4/5P2/6k1/2p5/2nB1Kr1/8/3nb3 w - -";
    test.boardFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    std::cout << test.isInCheck() << "\n";
    test.outputMoves=false;
    auto start = std::chrono::system_clock::now();
    chess::board test2;
    test2.boardFromFen("5rk1/r1p3pp/P7/2p5/1b2p3/4B2P/5P2/R1R3K1 w - - 2 24");
    chess::board test3;
    //n = perftTest(test, 4, 4);
    for(int i=0; i<99999; i++){
        test3 = test;
        test = test2;
        test2 = test3;
    }
    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << std::endl << "-----------------" << std::endl;
    std::cout << elapsed.count()/33333.0 << std::endl;
    std::cout << n << "\n";
    //std::cout << isInCheckCalls << "\n";
    //std::cout << times[0];
    /*initialise_all_databases();
    zobrist::initialise_zobrist_keys();
	
    Position p;
    Position::set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -", p);
    std::cout << p; 
    
    MoveList<WHITE> list(p);
    search<BLACK>(p);
    for(Move m : list) {
        std::cout << m << "\n";
    }*/
    
    return 0;
}