#include <cstdint>
#include <bitboards.h>
#include <rays.h>

//Everything here is heavily inspired by (and a lot of the time uses code from) https://github.com/GunshipPenguin/shallow-blue
//basically generate lookup tables so we can find all the moves of a given pieces without having to calculate
namespace lookupTables{

//Magic Numbers for Magic Bitboards. Magic Numbers are from https://github.com/GunshipPenguin/shallow-blue/blob/master/src/attacks.h
//Explained really well here: https://rhysre.net/fast-chess-move-generation-with-magic-bitboards.html
const uint64_t rookMagics[64] = {
    0xa8002c000108020ULL, 0x6c00049b0002001ULL, 0x100200010090040ULL, 0x2480041000800801ULL, 0x280028004000800ULL,
    0x900410008040022ULL, 0x280020001001080ULL, 0x2880002041000080ULL, 0xa000800080400034ULL, 0x4808020004000ULL,
    0x2290802004801000ULL, 0x411000d00100020ULL, 0x402800800040080ULL, 0xb000401004208ULL, 0x2409000100040200ULL,
    0x1002100004082ULL, 0x22878001e24000ULL, 0x1090810021004010ULL, 0x801030040200012ULL, 0x500808008001000ULL,
    0xa08018014000880ULL, 0x8000808004000200ULL, 0x201008080010200ULL, 0x801020000441091ULL, 0x800080204005ULL,
    0x1040200040100048ULL, 0x120200402082ULL, 0xd14880480100080ULL, 0x12040280080080ULL, 0x100040080020080ULL,
    0x9020010080800200ULL, 0x813241200148449ULL, 0x491604001800080ULL, 0x100401000402001ULL, 0x4820010021001040ULL,
    0x400402202000812ULL, 0x209009005000802ULL, 0x810800601800400ULL, 0x4301083214000150ULL, 0x204026458e001401ULL,
    0x40204000808000ULL, 0x8001008040010020ULL, 0x8410820820420010ULL, 0x1003001000090020ULL, 0x804040008008080ULL,
    0x12000810020004ULL, 0x1000100200040208ULL, 0x430000a044020001ULL, 0x280009023410300ULL, 0xe0100040002240ULL,
    0x200100401700ULL, 0x2244100408008080ULL, 0x8000400801980ULL, 0x2000810040200ULL, 0x8010100228810400ULL,
    0x2000009044210200ULL, 0x4080008040102101ULL, 0x40002080411d01ULL, 0x2005524060000901ULL, 0x502001008400422ULL,
    0x489a000810200402ULL, 0x1004400080a13ULL, 0x4000011008020084ULL, 0x26002114058042ULL
};
const uint64_t bishopMagics[64] = {
    0x89a1121896040240ULL, 0x2004844802002010ULL, 0x2068080051921000ULL, 0x62880a0220200808ULL, 0x4042004000000ULL,
    0x100822020200011ULL, 0xc00444222012000aULL, 0x28808801216001ULL, 0x400492088408100ULL, 0x201c401040c0084ULL,
    0x840800910a0010ULL, 0x82080240060ULL, 0x2000840504006000ULL, 0x30010c4108405004ULL, 0x1008005410080802ULL,
    0x8144042209100900ULL, 0x208081020014400ULL, 0x4800201208ca00ULL, 0xf18140408012008ULL, 0x1004002802102001ULL,
    0x841000820080811ULL, 0x40200200a42008ULL, 0x800054042000ULL, 0x88010400410c9000ULL, 0x520040470104290ULL,
    0x1004040051500081ULL, 0x2002081833080021ULL, 0x400c00c010142ULL, 0x941408200c002000ULL, 0x658810000806011ULL,
    0x188071040440a00ULL, 0x4800404002011c00ULL, 0x104442040404200ULL, 0x511080202091021ULL, 0x4022401120400ULL,
    0x80c0040400080120ULL, 0x8040010040820802ULL, 0x480810700020090ULL, 0x102008e00040242ULL, 0x809005202050100ULL,
    0x8002024220104080ULL, 0x431008804142000ULL, 0x19001802081400ULL, 0x200014208040080ULL, 0x3308082008200100ULL,
    0x41010500040c020ULL, 0x4012020c04210308ULL, 0x208220a202004080ULL, 0x111040120082000ULL, 0x6803040141280a00ULL,
    0x2101004202410000ULL, 0x8200000041108022ULL, 0x21082088000ULL, 0x2410204010040ULL, 0x40100400809000ULL,
    0x822088220820214ULL, 0x40808090012004ULL, 0x910224040218c9ULL, 0x402814422015008ULL, 0x90014004842410ULL,
    0x1000042304105ULL, 0x10008830412a00ULL, 0x2520081090008908ULL, 0x40102000a0a60140ULL,
};

//amount of positions for blockers. Excludes edges. Read the article on fast chess move generation with magic bitboards above for more information.
const int rookIndexBits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12
};
const int bishopIndexBits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6
};

//the lookup tables
uint64_t pawnAttackTable[2][64] = {{0}}; //need a table for white and black
uint64_t pawnPushTable[2][64] = {{0}};
uint64_t knightTable[64] = {0};
uint64_t kingTable[64] = {0};
//rook & bishop need to take other pieces("blockers") into account
uint64_t rookTable[64][4096] = {0}; 
uint64_t bishopTable[64][1024] = {0};

//rook & bishop need a prliminary mask array which doesn't take blockers into account
uint64_t rookMasks[64] = {0};
uint64_t bishopMasks[64] = {0};


//functions which initialize lookup tables for all pieces other than queen; queen simple uses both rook and bishop lookup tables
constexpr void initPawnTable();
constexpr void initKnightTable();
constexpr void initKingTable();
constexpr void initRookTable();
constexpr void initBishopTable();

//used to pregenerate moves to initialize lookup tables
constexpr uint64_t pregenerateRookMoves(int square, uint64_t blockers);
constexpr uint64_t pregenerateBishopMoves(int square, uint64_t blockers);

//initialize lookup tables
constexpr void init(){
    initPawnTable();
    initKnightTable();
    initKingTable();
    initRookTable();
    initBishopTable();
}

constexpr void initPawnTable(){
    for (int i = 0; i < 64; i++) {
    uint64_t pawnSquare = 1 << i;

    uint64_t whitePawnPushBb = (pawnSquare >> 8) | ((pawnSquare >> 16) & ~bitboards::rank4);
    uint64_t blackPawnPushBb = (pawnSquare << 8) | ((pawnSquare << 16) & ~bitboards::rank5);

    pawnPushTable[0][i] = whitePawnPushBb;
    pawnPushTable[1][i] = blackPawnPushBb;

    uint64_t whitePawnAttackBb = ((pawnSquare >> 9) & ~bitboards::fileA) | ((pawnSquare >> 7) & ~bitboards::fileH);
    uint64_t blackPawnAttackBb = ((pawnSquare << 9) & ~bitboards::fileH) | ((pawnSquare << 7) & ~bitboards::fileA);

    pawnAttackTable[0][i] = whitePawnAttackBb;
    pawnAttackTable[1][i] = blackPawnAttackBb;
    }
}
constexpr void initKnightTable(){
    for (int i = 0; i < 64; i++) {
    uint64_t knightSquare = 1 << i;

    uint64_t knightBb = (((knightSquare << 15) | (knightSquare >> 17)) & ~bitboards::fileA) | 
        (((knightSquare >> 15) | (knightSquare << 17)) & ~bitboards::fileH) | 
        (((knightSquare << 6) | (knightSquare >> 10)) & ~(bitboards::fileG | bitboards::fileH)) | 
        (((knightSquare >> 6) | (knightSquare << 10)) & ~(bitboards::fileA | bitboards::fileB)); 

    knightTable[i] = knightBb;
    }
}
constexpr void initKingTable(){
    for (int i = 0; i < 64; i++) {
    uint64_t kingSquare = 1 << i;

    uint64_t kingBb = (((kingSquare << 7) | (kingSquare >> 9) | (kingSquare >> 1)) & (~bitboards::fileA)) |
        (((kingSquare << 9) | (kingSquare >> 7) | (kingSquare << 1)) & (~bitboards::fileH)) |
        ((kingSquare >> 8) | (kingSquare << 8));

    kingTable[i] = kingBb;
    }
}

//code modified from https://github.com/GunshipPenguin/shallow-blue/blob/master/src/attacks.cc
constexpr uint64_t getBlockersFromIndex(int index, uint64_t mask) {
    uint64_t blockers = 0;
    int bits = __builtin_popcountll(mask);
    for (int i = 0; i < bits; i++) {
    int bitPos = __builtin_ffsll(mask) - 1;
    mask &= mask - 1;
    if (index & (1 << i)) {
        blockers |= (1 << bitPos);
    }
    }
    return blockers;
}

constexpr void initRookTable() {
    for (int rank = 0; rank < 8; rank++) {
        for(int file = 0; file < 8; file++){
            int square = rank*8+file;
            rookMasks[square] = ((bitboards::files[file] & ~bitboards::rank8 & ~bitboards::ranks[rank] & ~bitboards::rank1) |
                (bitboards::ranks[rank] & ~bitboards::fileH & ~bitboards::files[file] & ~bitboards::fileA));
        }
    }
    // For all squares
    for (int square = 0; square < 64; square++) {
    // For all possible blockers for this square
    for (int blockerIndex = 0; blockerIndex < (1 << rookIndexBits[square]); blockerIndex++) {
        uint64_t blockers = getBlockersFromIndex(blockerIndex, rookMasks[square]);
        rookTable[square][(blockers * rookMagics[square]) >> (64 - rookIndexBits[square])] =
            pregenerateBishopMoves(square, blockers);
    }
    }
}

constexpr void BishopTable() {
    for (int rank = 0; rank < 8; rank++) {
        for(int file = 0; file < 8; file++){
            int square = rank*8+file;

            for(int i=1; i+file<7 && i+rank<7; i++){bishopMasks[square] |= bitboards::files[file+i] & bitboards::ranks[rank+i];}
            for(int i=1; i+file<7 && rank-i>0; i++){bishopMasks[square] |= bitboards::files[file+i] & bitboards::ranks[rank+i];}
            for(int i=1; file-i>0 && i+rank<7; i++){bishopMasks[square] |= bitboards::files[file+i] & bitboards::ranks[rank+i];}
            for(int i=1; file-i>0 && rank-i>0; i++){bishopMasks[square] |= bitboards::files[file+i] & bitboards::ranks[rank+i];}
        }
    }
    // For all squares
    for (int square = 0; square < 64; square++) {
    // For all possible blockers for this square
    for (int blockerIndex = 0; blockerIndex < (1 << bishopIndexBits[square]); blockerIndex++) {
        uint64_t blockers = getBlockersFromIndex(blockerIndex, bishopMasks[square]);
        bishopTable[square][(blockers * bishopMagics[square]) >> (64 - bishopIndexBits[square])] =
            pregenerateBishopMoves(square, blockers);
    }
    }
}


//alright this part is basically completely copied from https://github.com/GunshipPenguin/shallow-blue/blob/master/src/bitutils.cc and https://github.com/GunshipPenguin/shallow-blue/blob/master/src/attacks.cc
constexpr inline int _bitscanForward(uint64_t board) {
  if (board == 0) {
    return -1;
  }
  return __builtin_ffsll(board) - 1;
}

constexpr inline int _bitscanReverse(uint64_t board) {
  if (board == 0) {
    return -1;
  }
  return 63 - __builtin_clzll(board);
}

constexpr uint64_t pregenerateBishopMoves(int square, uint64_t blockers) {
    uint64_t attacks = 0;

    // North West
    attacks |= rays::rays[4][square];
    if (rays::rays[4][square] & blockers) {
    attacks &= ~(rays::rays[4][_bitscanForward(rays::rays[4][square] & blockers)]);
    }

    // North East
    attacks |= rays::rays[5][square];
    if (rays::rays[5][square] & blockers) {
    attacks &= ~(rays::rays[5][_bitscanForward(rays::rays[5][square] & blockers)]);
    }

    // South East
    attacks |= rays::rays[6][square];
    if (rays::rays[6][square] & blockers) {
    attacks &= ~(rays::rays[6][_bitscanReverse(rays::rays[6][square] & blockers)]);
    }

    // South West
    attacks |= rays::rays[7][square];
    if (rays::rays[7][square] & blockers) {
    attacks &= ~(rays::rays[7][_bitscanReverse(rays::rays[7][square] & blockers)]);
    }

    return attacks;
}

constexpr uint64_t pregenerateRookMoves(int square, uint64_t blockers) {
    uint64_t attacks = 0;

    // North
    attacks |= rays::rays[0][square];
    if (rays::rays[0][square] & blockers) {
    attacks &= ~(rays::rays[0][_bitscanForward(rays::rays[0][square] & blockers)]);
    }

    // South
    attacks |= rays::rays[1][square];
    if (rays::rays[1][square] & blockers) {
    attacks &= ~(rays::rays[1][_bitscanReverse(rays::rays[1][square] & blockers)]);
    }

    // East
    attacks |= rays::rays[2][square];
    if (rays::rays[2][square] & blockers) {
    attacks &= ~(rays::rays[2][_bitscanForward(rays::rays[2][square] & blockers)]);
    }

    // West
    attacks |= rays::rays[3][square];
    if (rays::rays[3][square] & blockers) {
    attacks &= ~(rays::rays[3][_bitscanReverse(rays::rays[3][square] & blockers)]);
    }

    return attacks;
}
}
