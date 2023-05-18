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
int perftTest(board::board &currentBoard, int depth, int maxDepth){ //maxDepth for debug/move breakdown only
    int nodes = 0;
    if(depth==maxDepth){
        currentBoard.outputMoves = true;
    }
    else{
        currentBoard.outputMoves = false;
    }
    std::vector<board::board> legalMoves = currentBoard.generateMoves();
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
    board::board test;
    std::string h = "8/3N4/5P2/6k1/2p5/2nB1Kr1/8/3nb3 w - -";
    test.boardFromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    //std::cout << test.isInCheck();
    test.outputMoves=false;
    auto start = std::chrono::system_clock::now();
    std::pair<int, int> x = std::make_pair(0, 0);
    //n = perftTest(test, 4, 4);
    for(int i=0; i<100000; i++){
        test.makeMove(0, 0, x, false);
    }
    auto end = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << std::endl << "-----------------" << std::endl;
    std::cout << elapsed.count() << std::endl;
    std::cout << n << "\n";
    std::cout << isInCheckCalls << "\n";
    std::cout << times[432];
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