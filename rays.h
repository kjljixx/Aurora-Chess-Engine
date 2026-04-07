#pragma once
//Start reading code relating to move generation/chess rules in "bitboards.h"
//Then go to "lookup.h"
#include <cstdint>
#include "bitboards.h"

enum Direction{NORTH, SOUTH, EAST, WEST, NORTHWEST, NORTHEAST, SOUTHWEST, SOUTHEAST};

//this file is basically completely copied from https://github.com/GunshipPenguin/shallow-blue/blob/master/src/rays.cc
namespace rays{

inline U64 rays[8][64] = {{0}};

inline U64 _eastN(U64 board, int n) {
  U64 newBoard = board;
  for (int i = 0; i < n; i++) {
  newBoard = ((newBoard << 1) & (~bitboards::fileA));
  }

  return newBoard;
}

inline U64 _westN(U64 board, int n) {
  U64 newBoard = board;
  for (int i = 0; i < n; i++) {
  newBoard = ((newBoard >> 1) & (~bitboards::fileH));
  }

  return newBoard;
}

inline void init() {
  for (int square = 0; square < 64; square++) {
  // North
  rays[0][square] = 0x0101010101010100ULL << square;

  // South
  rays[1][square] = 0x0080808080808080ULL >> (63 - square);

  // East
  rays[2][square] = 2 * ((1ULL << (square | 7)) - (1ULL << square));

  // West
  rays[3][square] = (1ULL << square) - (1ULL << (square & 56));

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