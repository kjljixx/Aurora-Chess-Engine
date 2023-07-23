#include <cstdint>
#include "lookup.h"
#include <sstream>

namespace chess{

enum colors{WHITE, BLACK};

struct board{
  //bitboards for each piece of each color
  U64 wPawns; U64 bPawns;
  U64 wKnights; U64 bKnights;
  U64 wBishops; U64 bBishops;
  U64 wRooks; U64 bRooks;
  U64 wQueens; U64 bQueens;
  U64 wKing; U64 bKing;

  //bitboards for all pieces of each color
  U64 white;
  U64 black;

  //bitboard for all pieces on the board
  U64 occupied;

  //side to move
  colors sideToMove = WHITE;

  //castling rights
  unsigned char castlingRights; //each of the 4 bits in the char represents a sides castling rights. White Kingside is 0th bit from the right, White Queenside is 1st bit from the right, Black Kingside is 2nd, and Black Queenside is 3rd bit

  //en passant square
  U64 enPassant;

  //halfmove clock for 50 move rule
  int halfmoveClock;

  //constructor
  constexpr board(
  U64 wPawns, U64 bPawns,
  U64 wKnights, U64 bKnights,
  U64 wBishops, U64 bBishops,
  U64 wRooks, U64 bRooks,
  U64 wQueens, U64 bQueens,
  U64 wKing, U64 bKing,
  colors sideToMove, unsigned char castlingRights, U64 enPassant, int halfmoveClock) :

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
  sideToMove(sideToMove),
  castlingRights(castlingRights),
  enPassant(enPassant),
  halfmoveClock(halfmoveClock)
  {
  }

  board(){
    setToFen("K7/8/8/8/8/8/8/8 w - - 0 1");
  }

void setToFen(std::string fenString) {
  std::istringstream fenStream(fenString);
  std::string token;

  board(0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, 0ULL, WHITE, 0, 0, 0);

  U64 boardPos = 56; // Fen string starts at a8 = index 56
  fenStream >> token;
  for (auto currChar : token) {
  switch (currChar) {
    case 'p': bPawns |= (1ULL << boardPos++);
    break;
    case 'r': bRooks |= (1ULL << boardPos++);
    break;
    case 'n': bKnights |= (1ULL << boardPos++);
    break;
    case 'b': bBishops |= (1ULL << boardPos++);
    break;
    case 'q': bQueens |= (1ULL << boardPos++);
    break;
    case 'k': bKing |= (1ULL << boardPos++);
    break;
    case 'P': wPawns |= (1ULL << boardPos++);
    break;
    case 'R': wRooks |= (1ULL << boardPos++);
    break;
    case 'N': wKnights |= (1ULL << boardPos++);
    break;
    case 'B': wBishops |= (1ULL << boardPos++);
    break;
    case 'Q': wQueens |= (1ULL << boardPos++);
    break;
    case 'K': wKing |= (1ULL << boardPos++);
    break;
    case '/': boardPos -= 16; // Go down 1ULL rank
    break;
    default:boardPos += static_cast<U64>(currChar - '0');
  }
  }

  // Next to move
  fenStream >> token;
  sideToMove = token == "w" ? WHITE : BLACK;

  // Castling rights
  fenStream >> token;
  castlingRights = 0;
  for (auto currChar : token) {
    switch (currChar) {
      case 'K': castlingRights |= 0x1;
        break;
      case 'Q': castlingRights |= 0x2;
        break;
      case 'k': castlingRights |= 0x4;
        break;
      case 'q': castlingRights |= 0x8;
        break;
    }
  }

  // En passant target square
  fenStream >> token;
  enPassant = token == "-" ? 0ULL : 1ULL << squareNotationToIndex(token);

  // Halfmove clock
  fenStream >> halfmoveClock;

}

};

//used for move generation to make sure we aren't moving pieces of opponent color onto another piece of the same color
constexpr U64 opponentOrEmpty(board board){
  if(board.sideToMove == WHITE){return ~board.white;}

  return ~board.black;
}

constexpr U64 generateKingMoves(board board){
  if(board.sideToMove == WHITE){return lookupTables::kingTable[_bitscanForward(board.wKing)] & opponentOrEmpty(board);}

  return lookupTables::kingTable[_bitscanForward(board.bKing)] & opponentOrEmpty(board);
}

constexpr U64 generateKnightMoves(board board){
  if(board.sideToMove == WHITE){return lookupTables::knightTable[_bitscanForward(board.wKing)] & opponentOrEmpty(board);}

  return lookupTables::knightTable[_bitscanForward(board.bKing)] & opponentOrEmpty(board);
}

}//namespace chess