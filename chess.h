#include <cstdint>
#include "lookup.h"

namespace chess{

enum colors{WHITE, BLACK};

struct board{
    //bitboards for each piece of each color
    uint64_t wPawns; uint64_t bPawns;
    uint64_t wKnights; uint64_t bKnights;
    uint64_t wBishops; uint64_t bBishops;
    uint64_t wRooks; uint64_t bRooks;
    uint64_t wQueens; uint64_t bQueens;
    uint64_t wKing; uint64_t bKing;

    //bitboards for all pieces of each color
    uint64_t white;
    uint64_t black;

    //bitboard for all pieces on the board
    uint64_t occupied;

    colors sideToMove = WHITE;

    //constructor
    constexpr board(
    uint64_t wPawns, uint64_t bPawns,
    uint64_t wKnights, uint64_t bKnights,
    uint64_t wBishops, uint64_t bBishops,
    uint64_t wRooks, uint64_t bRooks,
    uint64_t wQueens, uint64_t bQueens,
    uint64_t wKing, uint64_t bKing,
    colors sideToMove) :

    //initializer list
    wPawns(wPawns), bPawns(bPawns),
    wKnights(wKnights), bKnights(bKnights),
    wBishops(wBishops), bBishops(bBishops),
    wRooks(wRooks), bRooks(bRooks),
    wQueens(wQueens), bQueens(bQueens),
    wKing(wKing), bKing(bKing),
    white(wPawns | wKnights | wBishops | wRooks | wQueens | wKing),
    black(bPawns | bKnights | bBishops | bBishops | bQueens | bKing),
    occupied(white | black),
    sideToMove(sideToMove)
    {
    }
};

//used for move generation to make sure we aren't moving pieces of one color onto another piece of the same color
constexpr uint64_t opponentOrEmpty(board board){
    if(board.sideToMove == WHITE){return ~board.white;}
    return ~board.black;
}
}//namespace chess*/