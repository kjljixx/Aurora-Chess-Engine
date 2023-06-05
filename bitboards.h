#include <cstdint>
#include <iostream>
//creates some basic bitboards and some basic functions for dealing with them
namespace bitboards{
//a1 is the 0th bit, b1 is the 1st, c1 is the 2nd, then a2 is the 8th, etc.
const uint64_t rank1 = 0xFF00000000000000ULL;
const uint64_t rank2 = 0xFF000000000000ULL;
const uint64_t rank3 = 0xFF0000000000ULL;
const uint64_t rank4 = 0xFF00000000ULL;
const uint64_t rank5 = 0xFF000000ULL;
const uint64_t rank6 = 0xFF0000ULL;
const uint64_t rank7 = 0xFF00ULL;
const uint64_t rank8 = 0xFFULL;

const uint64_t ranks[8] = {rank1, rank2, rank3, rank4, rank5, rank6, rank7, rank8};

const uint64_t fileA = 0x8080808080808080ULL;
const uint64_t fileB = 0x4040404040404040ULL;
const uint64_t fileC = 0x2020202020202020ULL;
const uint64_t fileD = 0x1010101010101010ULL;
const uint64_t fileE = 0x808080808080808ULL;
const uint64_t fileF = 0x404040404040404ULL;
const uint64_t fileG = 0x202020202020202ULL;
const uint64_t fileH = 0x101010101010101ULL;

const uint64_t files[8] = {fileA, fileB, fileC, fileD, fileE, fileF, fileG, fileH};

//"northeast" diagonals are defined by the rank of the square which is both on the diagonal and on the A file
//"northwest" diagonals are defined by the rank of the square on the diagonal and the H file

void printBoard(uint64_t board){
    uint64_t mask = 0;
    std::cout << "\n  A B C D E F G H";
    for(int i=0; i<8; i++){
        std::cout << "\n" << i+1 << " ";
        for(int j=0; j<8; j++){
            mask = 0;
            mask |= 1 << i*8+j;
            if(mask & board){std::cout << 1 << " ";}
            else{std::cout << 0 << " ";}
        }
    }
}

}