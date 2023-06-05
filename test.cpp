/*#include "src\position.h"
#include "src\tables.h"
#include "src\types.h"
#include "src\position.cpp"
#include "src\tables.cpp"
#include "src\types.cpp"*/
#include <vector>
#include <iostream>
#include "lookup.h"
#include "bitboards.h"

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
/*int perftTest(chess::board &currentBoard, int depth, int maxDepth){ //maxDepth for debug/move breakdown only
    //chess::board test;
    int nodes = 0;
    std::vector<chess::move> legalMoves = currentBoard.generateMoves();
    //if(depth==1){return legalMoves.size();}
    if(depth==0){return 1;}
    for(int i=0; i<legalMoves.size(); i++){
        //test = currentBoard;
        currentBoard.makeMove(legalMoves[i]);
        //std::cout << "{";
        int result = perftTest(currentBoard, depth-1, maxDepth);
        //std::cout << "}";
        currentBoard.unMakeLastMove(legalMoves[i]);
        if(depth==maxDepth){
            legalMoves[i].printMove();
            std::cout << ":" << result << "\n";
        }
        //std::cout << (currentBoard.fullBoard == test.fullBoard);
        nodes += result;
    }
    return nodes;
}*/


int main() {
    lookupTables::init();
    bitboards::printBoard(lookupTables::kingTable[36]);
    return 0;
}