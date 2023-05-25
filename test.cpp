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
    //chess::board test;
    int nodes = 0;
    if(depth==maxDepth){
        currentBoard.outputMoves = true;
    }
    else{
        currentBoard.outputMoves = false;
    }
    std::vector<chess::move> legalMoves = currentBoard.generateMoves();
    if(depth==1){return legalMoves.size();}
    for(int i=0; i<legalMoves.size(); i++){
        //test = currentBoard;
        currentBoard.makeMove(legalMoves[i]);
        //std::cout << "{";
        int result = perftTest(currentBoard, depth-1, maxDepth);
        //std::cout << "}";
        currentBoard.unMakeLastMove(legalMoves[i]);
        if(depth==maxDepth){
            std::cout << result << "\n";
        }
        //std::cout << (currentBoard.fullBoard == test.fullBoard);
        nodes += result;
    }
    return nodes;
}


int main() {
    int n = 0;
    chess::board test;
    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    test.initialize(fen);
    test.outputMoves=false;
    std::vector<chess::board> testList;
    auto start = std::chrono::system_clock::now();
    n = perftTest(test, 5, 5);
    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << std::endl << "-----------------" << std::endl;
    std::cout << "Time in Microseconds:" << elapsed.count() << "\n";
    std::cout << "Nodes:" << n << "\n";
    std::cout << "Nodes per Second:" << std::fixed << n/(elapsed.count()/1000000.0) << "\n";
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