#include <cstdint>
#include <bitboards.h>


//this file is basically completely copied from https://github.com/GunshipPenguin/shallow-blue/blob/master/src/rays.cc
namespace rays{

uint64_t rays[8][64] = {0};

constexpr inline uint64_t _eastN(uint64_t board, int n) {
    uint64_t newBoard = board;
    for (int i = 0; i < n; i++) {
    newBoard = ((newBoard << 1) & (~bitboards::fileA));
    }

    return newBoard;
}

constexpr inline uint64_t _westN(uint64_t board, int n) {
    uint64_t newBoard = board;
    for (int i = 0; i < n; i++) {
    newBoard = ((newBoard >> 1) & (~bitboards::fileH));
    }

    return newBoard;
}

constexpr void init() {
    for (int square = 0; square < 64; square++) {
    // North
    rays[0][square] = 0x0101010101010100ULL << square;

    // South
    rays[1][square] = 0x0080808080808080ULL >> (63 - square);

    // East
    rays[2][square] = 2 * ((1 << (square | 7)) - (1 << square));

    // West
    rays[3][square] = (1 << square) - (1 << (square & 56));

    // North West
    rays[4][square] = _westN(0x102040810204000ULL, 7 - square % 8) << (square/8 * 8);

    // North East
    rays[5][square] = _eastN(0x8040201008040200ULL, square % 8) << (square/8 * 8);

    // South West
    rays[6][square] = _westN(0x40201008040201ULL, 7 - square % 8) >> ((7 - square/8) * 8);

    // South East
    rays[7][square] = _eastN(0x2040810204080ULL, square % 8) >> ((7 - square/8) * 8);
    }
}

}