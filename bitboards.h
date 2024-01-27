#pragma once
//Start reading code relating to move generation/chess rulesin "bitboards.h"(THIS FILE)
//After reading this file, go to "rays.h"
#include <cstdint>
#include <iostream>
#include <cassert>

using U64 = unsigned long long;

constexpr inline uint8_t _bitscanForward(U64 board) {
  assert(board);
  return __builtin_ctzll(board);
}

constexpr inline uint8_t _bitscanReverse(U64 board) {
  assert(board);
  return 63 - __builtin_clzll(board);
}

constexpr inline uint8_t _popCount(U64 board) {
  return __builtin_popcountll(board);
}

constexpr inline uint8_t _popLsb(U64 &board) {
  assert(board);
  const int lsbIndex = _bitscanForward(board);
  board &= board - 1;
  return lsbIndex;
}

inline U64 _flipBoard(U64 &board){
  return __builtin_bswap64(board);
}

//functions for transferring back and forth between notation(ex. a8) and index(ex. 56)
uint8_t squareNotationToIndex(std::string notation){
  return (notation[1] - '1')*8+(notation[0] - 'a');
}

std::string squareIndexToNotation(int index){
  return std::string(1, ('a' + index % 8))+std::string(1, ('1' + index / 8));
}

constexpr uint8_t squareIndexToRank(uint8_t index){return index / 8;}
constexpr uint8_t squareIndexToFile(uint8_t index){return index % 8;}



//creates some basic bitboards and some basic functions for dealing with them
namespace bitboards{
//a1 is the 0th bit from the right, b1 is the 1st, c1 is the 2nd, then a2 is the 8th, etc.
const U64 rank8 = 0xFF00000000000000ULL;
const U64 rank7 = 0xFF000000000000ULL;
const U64 rank6 = 0xFF0000000000ULL;
const U64 rank5 = 0xFF00000000ULL;
const U64 rank4 = 0xFF000000ULL;
const U64 rank3 = 0xFF0000ULL;
const U64 rank2 = 0xFF00ULL;
const U64 rank1 = 0xFFULL;

const U64 ranks[8] = {rank1, rank2, rank3, rank4, rank5, rank6, rank7, rank8};

const U64 fileH = 0x8080808080808080ULL;
const U64 fileG = 0x4040404040404040ULL;
const U64 fileF = 0x2020202020202020ULL;
const U64 fileE = 0x1010101010101010ULL;
const U64 fileD = 0x808080808080808ULL;
const U64 fileC = 0x404040404040404ULL;
const U64 fileB = 0x202020202020202ULL;
const U64 fileA = 0x101010101010101ULL;

const U64 files[8] = {fileA, fileB, fileC, fileD, fileE, fileF, fileG, fileH};

void printBoard(U64 board){
  U64 mask = 0;
  for(int i=8; i>0; i--){
    std::cout << "\n" << i << " ";
    for(int j=0; j<8; j++){
      mask = 0;
      mask |= (1ULL << (i-1)*8+j);
      if(mask & board){std::cout << "1 ";}
      else{std::cout << "0 ";}
    }
  }
  std::cout << "\n  A B C D E F G H";
}

}