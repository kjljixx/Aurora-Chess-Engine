//Start reading code relating to move generation/chess rules in "bitboards.h"
//After reading this file, go to "chess.h"
#include <cstdint>
#include <vector>
#include "rays.h"


//Everything here is heavily inspired by (and a lot of the time uses code from) https://github.com/GunshipPenguin/shallow-blue
//generate lookup tables so we can find all the moves of a given pieces without having to calculate it
namespace chess{
enum Colors{WHITE, BLACK};
enum Pieces{null, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, UNKNOWN};

Pieces letterToPiece(char letter){
  switch (letter){
    case 'p': return PAWN;
    case 'n': return KNIGHT;
    case 'b': return BISHOP;
    case 'r': return ROOK;
    case 'q': return QUEEN;
    case 'k': return KING;
  }
  return null;
}
char PieceToLetter(Pieces piece){ //mainly for PGN move notation
  switch (piece){
    case PAWN : return 'P';
    case KNIGHT: return 'N';
    case BISHOP: return 'B';
    case ROOK: return 'R';
    case QUEEN: return 'Q';
    case KING: return 'K';
  }
  return ' ';
}

enum MoveFlags{
  NONE, CASTLE = 1 << 14, ENPASSANT = 2 << 14, PROMOTION = 3 << 14
};

struct Move{
  //the internal value of the move (bits 0-5 end square, bits 6-11 start square, bits 12-13 move flags, bits 14-15 promotion piece type)
  uint16_t value;

  constexpr Move(uint8_t startSquare, uint8_t endSquare, MoveFlags moveFlags = NONE, Pieces promotionPiece = KNIGHT): 
    value(moveFlags + ((promotionPiece-KNIGHT) << 12) + (startSquare << 6) + (endSquare)){}
  //default constructor is a null move
  constexpr Move(): value(0){}
  constexpr uint8_t getStartSquare(){
    return (value & 0b0000111111000000) >> 6;
  }
  constexpr uint8_t getEndSquare(){
    return (value & 0b0000000000111111);
  }
  constexpr Pieces getPromotionPiece(){
    return Pieces(((value & 0b0011000000000000) >> 12)+KNIGHT);
  }
  constexpr MoveFlags getMoveFlags(){
    return MoveFlags(value & 0b1100000000000000);
  }
  
  //returns a string representation of the move (Pure Algebraic Coordinate Notation)
  std::string toStringRep(){
    std::string stringRep = squareIndexToNotation(getStartSquare())+squareIndexToNotation(getEndSquare());
    if(getMoveFlags() == PROMOTION){
      switch (getPromotionPiece()){
        case KNIGHT: return stringRep+"n"; break;
        case BISHOP: return stringRep+"b"; break;
        case ROOK: return stringRep+"r"; break;
        case QUEEN: return stringRep+"q"; break;
      }
    }
    return stringRep;
  }
};

constexpr Move* MoveListFromBitboard(U64 moves, uint8_t startSquare, bool isPawn, Move* movesList, MoveFlags moveFlags = NONE){
  if(moves == 0){return movesList;}
  if(isPawn && (moves & 0xFF000000000000FFULL))//checks if move is pawn promotion. FF000000000000FF is the first and eight ranks
  {while(moves){
    uint8_t endSquare = _popLsb(moves);
    *movesList++ = Move(startSquare, endSquare, PROMOTION, KNIGHT);
    *movesList++ = Move(startSquare, endSquare, PROMOTION, BISHOP);
    *movesList++ = Move(startSquare, endSquare, PROMOTION, ROOK);
    *movesList++ = Move(startSquare, endSquare, PROMOTION, QUEEN);
  }
  }
  else{
  while(moves){
    uint8_t endSquare = _popLsb(moves);
    *movesList++ = Move(startSquare, endSquare, moveFlags);
  }
  }
  return movesList;
}
}//namespace chess

namespace lookupTables{

//Magic numbers for magic bitboards. Explained really well here: https://rhysre.net/fast-chess-move-generation-with-magic-bitboards.html
const U64 rookMagics[64] = {
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
const U64 bishopMagics[64] = {
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
  0x1000042304105ULL, 0x10008830412a00ULL, 0x2520081090008908ULL, 0x40102000a0a60140ULL
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

//the bitboard lookup tables
U64 pawnAttackTable[2][64] = {{}}; //need a table for white and black
U64 pawnPushTable[2][64] = {{}};
U64 knightTable[64] = {};
U64 kingTable[64] = {};
//rook & bishop need to take other pieces("blockers") into account
U64 rookTable[64][4096] = {}; 
U64 bishopTable[64][1024] = {};

//rook & bishop need a preliminary mask array which doesn't take blockers into account
U64 rookMasks[64] = {0};
U64 bishopMasks[64] = {0};


//functions which initialize lookup tables for all pieces other than queen; queen simple uses both rook and bishop lookup tables
void initPawnTable();
void initKnightTable();
void initKingTable();
void initRookTable();
void initBishopTable();

//used to pregenerate moves to initialize lookup tables
U64 pregenerateRookMoves(int square, U64 blockers);
U64 pregenerateBishopMoves(int square, U64 blockers);

//initialize lookup tables
void init(){
  rays::init();
  initPawnTable();
  initKnightTable();
  initKingTable();
  initRookTable();
  initBishopTable();
}

void initPawnTable(){
  for (int i = 0; i < 64; i++) {
  U64 pawnSquare = 1ULL << i;

  U64 whitePawnPushBb = (pawnSquare << 8);
  U64 blackPawnPushBb = (pawnSquare >> 8);
  pawnPushTable[0][i] = whitePawnPushBb;
  pawnPushTable[1][i] = blackPawnPushBb;

  U64 whitePawnAttackBb = ((pawnSquare << 9) & ~bitboards::fileA) | ((pawnSquare << 7) & ~bitboards::fileH);
  U64 blackPawnAttackBb = ((pawnSquare >> 9) & ~bitboards::fileH) | ((pawnSquare >> 7) & ~bitboards::fileA);
  pawnAttackTable[0][i] = whitePawnAttackBb;
  pawnAttackTable[1][i] = blackPawnAttackBb;
  }
}
void initKnightTable(){
  for (int i = 0; i < 64; i++) {
  U64 knightSquare = 1ULL << i;
  U64 knightBb = (((knightSquare << 15) | (knightSquare >> 17)) & ~bitboards::fileH) | 
    (((knightSquare >> 15) | (knightSquare << 17)) & ~bitboards::fileA) | 
    (((knightSquare << 6) | (knightSquare >> 10)) & ~(bitboards::fileG | bitboards::fileH)) | 
    (((knightSquare >> 6) | (knightSquare << 10)) & ~(bitboards::fileA | bitboards::fileB)); 
  knightTable[i] = knightBb;
  }
}
void initKingTable(){
  for (int i = 0; i < 64; i++) {
  U64 kingSquare = 1ULL << i;

  U64 kingBb = (((kingSquare >> 7) | (kingSquare << 9) | (kingSquare << 1)) & (~bitboards::fileA)) |
    (((kingSquare >> 9) | (kingSquare << 7) | (kingSquare >> 1)) & (~bitboards::fileH)) |
    ((kingSquare >> 8) | (kingSquare << 8));
  kingTable[i] = kingBb;
  }
}

U64 getBlockersFromIndex(int index, U64 mask) {
  U64 blockers = 0;
  int bits = _popCount(mask);
  for (int i = 0; i < bits; i++) {
  int bitPos = _popLsb(mask);
  if (index & (1ULL << i)) {
    blockers |= (1ULL << bitPos);
  }
  }
  return blockers;
}

void initRookTable() {
  for (int square = 0; square < 64; square++) {
    rookMasks[square] = (rays::rays[0][square] & ~bitboards::rank8) |
    (rays::rays[1][square] & ~bitboards::rank1) |
    (rays::rays[2][square] & ~bitboards::fileH) |
    (rays::rays[3][square] & ~bitboards::fileA);
  }
  // For all squares
  for (int square = 0; square < 64; square++) {
  // For all possible blockers for this square
  for (int blockerIndex = 0; blockerIndex < (1ULL << rookIndexBits[square]); blockerIndex++) {
    U64 blockers = getBlockersFromIndex(blockerIndex, rookMasks[square]);

    rookTable[square][(blockers * rookMagics[square]) >> (64 - rookIndexBits[square])] =
      pregenerateRookMoves(square, blockers);
  }
  }
}

void initBishopTable() {
  U64 edgeSquares = bitboards::fileA | bitboards::fileH | bitboards::rank1 | bitboards::rank8;
  for (int square = 0; square < 64; square++) {
    bishopMasks[square] = (rays::rays[4][square] | rays::rays[5][square] |
    rays::rays[6][square] | rays::rays[7][square]) & ~(edgeSquares);
  }
  // For all squares
  for (int square = 0; square < 64; square++) {
  // For all possible blockers for this square
  for (int blockerIndex = 0; blockerIndex < (1ULL << bishopIndexBits[square]); blockerIndex++) {
    U64 blockers = getBlockersFromIndex(blockerIndex, bishopMasks[square]);

    bishopTable[square][(blockers * bishopMagics[square]) >> (64 - bishopIndexBits[square])] =
      pregenerateBishopMoves(square, blockers);
  }
  }
}

U64 pregenerateBishopMoves(int square, U64 blockers) {
  U64 attacks = 0;

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

U64 pregenerateRookMoves(int square, U64 blockers) {
  U64 attacks = 0;

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

inline U64 getBishopAttacks(uint8_t square, U64 blockers) {
  return bishopTable[square][((blockers & bishopMasks[square]) * bishopMagics[square]) >> (64 - bishopIndexBits[square])];
}

inline U64 getRookAttacks(uint8_t square, U64 blockers) {
  return rookTable[square][((blockers & rookMasks[square]) * rookMagics[square]) >> (64 - rookIndexBits[square])];
}

}
