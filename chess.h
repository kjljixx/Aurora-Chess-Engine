#pragma once
//Start reading code relating to move generation/chess rules in "bitboards.h"
//After reading this file, go to "uci.h"
#include <cstdint>
#include "lookup.h"
//#include "Fathom-1.0/src/tbprobe.h"
#include <sstream>
#include <vector>
#include <algorithm>
#include <array>

namespace chess{

const std::string startPosFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

struct KingMasks{
  U64 checkmask;
  U64 rookPinmask;
  U64 rookPinnedPieces;
  U64 bishopPinmask;
  U64 bishopPinnedPieces;

  KingMasks():
  checkmask(0ULL),
  rookPinmask(0ULL), rookPinnedPieces(0ULL),
  bishopPinmask(0ULL), bishopPinnedPieces(0ULL)
  {}
};

struct Board{
  //bitboards for each piece of each color
  U64 pawns;
  U64 knights;
  U64 bishops;
  U64 rooks;
  U64 queens;
  U64 kings;
  //bitboards for all Pieces of each color
  U64 white;
  U64 black;
  //bitboard for all Pieces on the board
  U64 occupied;
  Colors sideToMove = WHITE; unsigned char castlingRights; U64 enPassant; int halfmoveClock;

  unsigned char canCurrentlyCastle;

  //use to check for repetitions
  U64 history[128];
  bool hashed; //if there is a zobrist hash of the position in history, this is true
  uint8_t startHistoryIndex; //When a fen is entered, we want getGameStatus to ignore all items of history before the halfmoveClock of the fen

  std::array<std::array<uint8_t, 64>, 2> mailbox; //mailbox representation of the board, one for each side

  //constructor
  Board(
  U64 pawns,
  U64 knights,
  U64 bishops,
  U64 rooks,
  U64 queens,
  U64 kings,
  U64 white,
  U64 black,
  Colors sideToMove, unsigned char castlingRights, U64 enPassant, int halfmoveClock) :

  //initializer list
  pawns(pawns),
  knights(knights),
  bishops(bishops),
  rooks(rooks),
  queens(queens),
  kings(kings),
  white(white),
  black(black),
  occupied(white | black),
  sideToMove(sideToMove),
  castlingRights(castlingRights),
  enPassant(enPassant),
  halfmoveClock(halfmoveClock),
  hashed(false)
  {
    for(int i=0; i<64; i++){mailbox[0][i] = 0; mailbox[1][i] = 0;}
  }

  Board(){
    for(int i=0; i<64; i++){mailbox[0][i] = 0; mailbox[1][i] = 0;}
    setToFen(startPosFen);
    hashed = false;
  }

  Board(std::string fen){
    for(int i=0; i<64; i++){mailbox[0][i] = 0; mailbox[1][i] = 0;}
    setToFen(fen);
    hashed = false;
  }

  //Detects if all pieces are in the same position between two boards. THIS ONLY ACCOUNTS FOR PIECES BEING IN SPECIFIC SQUARES AND SIDE TO MOVE, NOTHING ELSE.
  bool operator==(const Board& board){
    return (
      pawns == board.pawns &&
      knights == board.knights &&
      bishops == board.bishops &&
      rooks == board.rooks &&
      queens == board.queens &&
      kings == board.kings &&
      mailbox == board.mailbox &&
      sideToMove == board.sideToMove
    );
  }

  void setToFen(std::string fenString) {
    std::istringstream fenStream(fenString);
    std::string token;

    for(int i=0; i<64; i++){mailbox[0][i] = 0; mailbox[1][i] = 0;}
    hashed = false;

    pawns=0ULL;
    knights=0ULL;
    bishops=0ULL;
    rooks=0ULL;
    queens=0ULL;
    kings=0ULL;
    white=0ULL;
    black=0ULL;
    occupied=0ULL;
    sideToMove=WHITE;
    castlingRights=0;
    enPassant=0ULL;
    halfmoveClock=0;

    uint8_t boardPos = 56; // Fen string starts at a8 = index 56
    fenStream >> token;
    for (auto currChar : token) {
    switch (currChar) {
      case 'p': mailbox[0][boardPos] = 7; mailbox[1][boardPos^56] = 1; pawns |= (1ULL << boardPos); black |= (1ULL << boardPos++);
      break;
      case 'r': mailbox[0][boardPos] = 10; mailbox[1][boardPos^56] = 4; rooks |= (1ULL << boardPos); black |= (1ULL << boardPos++);
      break;
      case 'n': mailbox[0][boardPos] = 8; mailbox[1][boardPos^56] = 2; knights |= (1ULL << boardPos); black |= (1ULL << boardPos++);
      break;
      case 'b': mailbox[0][boardPos] = 9; mailbox[1][boardPos^56] = 3; bishops |= (1ULL << boardPos); black |= (1ULL << boardPos++);
      break;
      case 'q': mailbox[0][boardPos] = 11; mailbox[1][boardPos^56] = 5; queens |= (1ULL << boardPos); black |= (1ULL << boardPos++);
      break;
      case 'k': mailbox[0][boardPos] = 12; mailbox[1][boardPos^56] = 6; kings |= (1ULL << boardPos); black |= (1ULL << boardPos++);
      break;
      case 'P': mailbox[0][boardPos] = 1; mailbox[1][boardPos^56] = 7; pawns |= (1ULL << boardPos); white |= (1ULL << boardPos++);
      break;
      case 'R': mailbox[0][boardPos] = 4; mailbox[1][boardPos^56] = 10; rooks |= (1ULL << boardPos); white |= (1ULL << boardPos++);
      break;
      case 'N': mailbox[0][boardPos] = 2; mailbox[1][boardPos^56] = 8; knights |= (1ULL << boardPos); white |= (1ULL << boardPos++);
      break;
      case 'B': mailbox[0][boardPos] = 3; mailbox[1][boardPos^56] = 9; bishops |= (1ULL << boardPos); white |= (1ULL << boardPos++);
      break;
      case 'Q': mailbox[0][boardPos] = 5; mailbox[1][boardPos^56] = 11; queens |= (1ULL << boardPos); white |= (1ULL << boardPos++);
      break;
      case 'K': mailbox[0][boardPos] = 6; mailbox[1][boardPos^56] = 12; kings |= (1ULL << boardPos); white |= (1ULL << boardPos++);
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

    //white, black, and occupied
    occupied = white | black;

    startHistoryIndex = halfmoveClock;
  }

  void printBoard(){
    U64 mask = 0;
    for(int i=8; i>0; i--){
      std::cout << "\n" << i << " ";
      for(int j=0; j<8; j++){
        mask = 0;
        mask |= (1ULL << ((i-1)*8+j));
        if(pawns & mask){std::cout << char('P'+((mask & black) != 0)*32) << " ";}
        else if(knights & mask){std::cout << char('N'+((mask & black) != 0)*32) << " ";}
        else if(bishops & mask){std::cout << char('B'+((mask & black) != 0)*32) << " ";}
        else if(rooks & mask){std::cout << char('R'+((mask & black) != 0)*32) << " ";}
        else if(queens & mask){std::cout << char('Q'+((mask & black) != 0)*32) << " ";}
        else if(kings & mask){std::cout << char('K'+((mask & black) != 0)*32) << " ";}
        else{std::cout << "- ";}
      }
    }
    std::cout << "\n  A B C D E F G H";
    std::cout << " HMC: " << halfmoveClock;
    std::cout << " STM: " << sideToMove;
  }

  std::string getFen(){
    std::string fen;
    U64 mask = 0;
    for(int i=8; i>0; i--){
      if(i != 8){fen += "/";}

      int noPieceCounter = 0;
      for(int j=0; j<8; j++){
        mask = 0;
        mask |= (1ULL << ((i-1)*8+j));

        if(occupied & mask){
          if(noPieceCounter){fen += std::to_string(noPieceCounter); noPieceCounter = 0;}
        }
        else if(j == 7){
          fen += std::to_string(noPieceCounter+1); noPieceCounter = 0;
        }

        if(pawns & mask){fen += char('P'+((mask & black) != 0)*32);}
        else if(knights & mask){fen += char('N'+((mask & black) != 0)*32);}
        else if(bishops & mask){fen += char('B'+((mask & black) != 0)*32);}
        else if(rooks & mask){fen += char('R'+((mask & black) != 0)*32);}
        else if(queens & mask){fen += char('Q'+((mask & black) != 0)*32);}
        else if(kings & mask){fen += char('K'+((mask & black) != 0)*32);}

        else{noPieceCounter++;}
      }
    }

    fen += " ";
    if(sideToMove){fen += "b";}
    else{fen += "w";}

    fen += " ";
    if(castlingRights){
      if(castlingRights & 0x1){
        fen += "K";
      }
      if(castlingRights & 0x2){
        fen += "Q";
      }
      if(castlingRights & 0x4){
        fen += "k";
      }
      if(castlingRights & 0x8){
        fen += "q";
      }
    }
    else{
      fen += "-";
    }

    fen += " ";
    if(enPassant){
      fen += squareIndexToNotation(_bitscanForward(enPassant));
    }
    else{
      fen += "-";
    }

    fen += " ";
    fen += std::to_string(halfmoveClock);

    return fen;
  }

  U64 getPieces(Colors color){
    if(color==WHITE){return white;}
    return black;
  }
  U64 getPieces(Colors color, Pieces piece){
    U64 _pieces = getPieces(color);
    switch (piece) {
      case PAWN: return pawns & _pieces; break;
      case KNIGHT: return knights & _pieces; break;
      case BISHOP: return bishops & _pieces; break;
      case ROOK: return rooks & _pieces; break;
      case QUEEN: return queens & _pieces; break;
      case KING: return kings & _pieces; break;
      default: assert(0);
    }
    return 0ULL;
  }

  U64 getOurPieces(){
    if(sideToMove==WHITE){return white;}
    return black;
  }
  U64 getOurPieces(Pieces piece){
    U64 _ourPieces = getOurPieces();
    switch (piece) {
      case PAWN: return pawns & _ourPieces; break;
      case KNIGHT: return knights & _ourPieces; break;
      case BISHOP: return bishops & _ourPieces; break;
      case ROOK: return rooks & _ourPieces; break;
      case QUEEN: return queens & _ourPieces; break;
      case KING: return kings & _ourPieces; break;
      default: assert(0);
    }
    return 0ULL;
  }

  U64 getTheirPieces(){
    if(sideToMove==WHITE){return black;}
    return white;
  }
  U64 getTheirPieces(Pieces piece){
    U64 _theirPieces = getTheirPieces();
    switch (piece) {
      case PAWN: return pawns & _theirPieces; break;
      case KNIGHT: return knights & _theirPieces; break;
      case BISHOP: return bishops & _theirPieces; break;
      case ROOK: return rooks & _theirPieces; break;
      case QUEEN: return queens & _theirPieces; break;
      case KING: return kings & _theirPieces; break;
      default: assert(0);
    }
    return 0ULL;
  }

  //sets a bit or unsets a bit
  void setColors(uint8_t bit, Colors color){
    if(color==WHITE){white |= (1ULL << bit);}
    else{black |= (1ULL << bit);}
  }
  void unsetColors(uint8_t bit, Colors color){
    if(color==WHITE){white &= ~(1ULL << bit);}
    else{black &= ~(1ULL << bit);}
  }
  void setColors(U64 bit, Colors color){
    if(color==WHITE){white |= bit;}
    else{black |= bit;}
  }
  void unsetColors(U64 bit, Colors color){
    if(color==WHITE){white &= ~bit;}
    else{black &= ~bit;}
  }
  void setPieces(Pieces piece, uint8_t bit){
    switch (piece) {
      case PAWN: pawns |= 1ULL << bit; break;
      case KNIGHT: knights |= 1ULL << bit; break;
      case BISHOP: bishops |= 1ULL << bit; break;
      case ROOK: rooks |= 1ULL << bit; break;
      case QUEEN: queens |= 1ULL << bit; break;
      case KING: kings |= 1ULL << bit; break;
      default: assert(0);
    }
  }
  void unsetPieces(Pieces piece, uint8_t bit){
    switch (piece) {
      case PAWN: pawns &= ~(1ULL << bit); break;
      case KNIGHT: knights &= ~(1ULL << bit); break;
      case BISHOP: bishops &= ~(1ULL << bit); break;
      case ROOK: rooks &= ~(1ULL << bit); break;
      case QUEEN: queens &= ~(1ULL << bit); break;
      case KING: kings &= ~(1ULL << bit); break;
      case UNKNOWN:{U64 unsetBitboard = ~(1ULL << bit); pawns &= unsetBitboard; knights &= unsetBitboard; bishops &= unsetBitboard; rooks &= unsetBitboard; queens &= unsetBitboard; kings &= unsetBitboard; break;}
      default: assert(0);
    }
  }
  void setPieces(Pieces piece, U64 bit){
    switch (piece) {
      case PAWN: pawns |= bit; break;
      case KNIGHT: knights |= bit; break;
      case BISHOP: bishops |= bit; break;
      case ROOK: rooks |= bit; break;
      case QUEEN: queens |= bit; break;
      case KING: kings |= bit; break;
      default: assert(0);
    }
  }
  void unsetPieces(Pieces piece, U64 bit){
    switch (piece) {
      case PAWN: pawns &= ~bit; break;
      case KNIGHT: knights &= ~bit; break;
      case BISHOP: bishops &= ~bit; break;
      case ROOK: rooks &= ~bit; break;
      case QUEEN: queens &= ~bit; break;
      case KING: kings &= ~bit; break;
      case UNKNOWN:{U64 unsetBitboard = ~bit; pawns &= unsetBitboard; knights &= unsetBitboard; bishops &= unsetBitboard; rooks &= unsetBitboard; queens &= unsetBitboard; kings &= unsetBitboard; break;}
      default: assert(0);
    }
  }

  //generates checkmasks and pinmasks
  KingMasks generateKingMasks(){
    U64 ourPieces = getOurPieces();
    uint8_t square = _bitscanForward(ourPieces & kings);
    U64 theirPieces = getTheirPieces();
    KingMasks _kingMasks;

    int numAttackers = 0;

    int squareRank = squareIndexToRank(square); int squareFile = squareIndexToFile(square);
    if(lookupTables::knightTable[square] & knights & theirPieces){_kingMasks.checkmask |= lookupTables::knightTable[square] & knights & theirPieces; numAttackers++;}

    U64 rookAttacks = lookupTables::getRookAttacks(square, theirPieces) & (rooks | queens) & theirPieces;
    U64 attackRay;
    while(rookAttacks){
      int theirRook = _popLsb(rookAttacks);
      int theirRookRank = squareIndexToRank(theirRook); int theirRookFile = squareIndexToFile(theirRook);
      if(theirRookRank==squareRank){
        if(theirRookFile>squareFile){attackRay = rays::rays[2][square] & ~rays::rays[2][theirRook];}
        else{attackRay = rays::rays[3][square] & ~rays::rays[3][theirRook];}
      }
      else{
        if(theirRookRank>squareRank){attackRay = rays::rays[0][square] & ~rays::rays[0][theirRook];}
        else{attackRay = rays::rays[1][square] & ~rays::rays[1][theirRook];}
      }
      if(!(attackRay & ourPieces)){_kingMasks.checkmask |= attackRay; numAttackers++;}
      else if(_popCount(attackRay & ourPieces) == 1){_kingMasks.rookPinmask |= attackRay; _kingMasks.rookPinnedPieces |= attackRay & ourPieces;}
    }

    U64 bishopAttacks = lookupTables::getBishopAttacks(square, theirPieces) & (bishops | queens) & theirPieces;
    while(bishopAttacks){
      int theirBishop = _popLsb(bishopAttacks);
      int theirBishopRank = squareIndexToRank(theirBishop); int theirBishopFile = squareIndexToFile(theirBishop);
      if(theirBishopRank>squareRank){
        if(theirBishopFile<squareFile){attackRay = rays::rays[4][square] & ~rays::rays[4][theirBishop];}
        else{attackRay = rays::rays[5][square] & ~rays::rays[5][theirBishop];}
      }
      else{
        if(theirBishopFile<squareFile){attackRay = rays::rays[6][square] & ~rays::rays[6][theirBishop];}
        else{attackRay = rays::rays[7][square] & ~rays::rays[7][theirBishop];}
      }
      if(!(attackRay & ourPieces)){_kingMasks.checkmask |= attackRay; numAttackers++;}
      else if(_popCount(attackRay & ourPieces) == 1){_kingMasks.bishopPinmask |= attackRay; _kingMasks.bishopPinnedPieces |= attackRay & ourPieces;}
    }
    if(lookupTables::pawnAttackTable[sideToMove][square] & pawns & theirPieces){_kingMasks.checkmask |= lookupTables::pawnAttackTable[sideToMove][square] & pawns & theirPieces; numAttackers++;}
    
    if(numAttackers>1){_kingMasks.checkmask = 0ULL;}
    if(numAttackers==0){_kingMasks.checkmask = 0xFFFFFFFFFFFFFFFFULL;}
    return _kingMasks;
  }
  //Returns the square of the enemy piece which is attacking the square, if there is one. Otherwise returns 64
  uint8_t squareUnderAttack(uint8_t square){
    U64 theirPieces = getTheirPieces();

    if(lookupTables::pawnAttackTable[sideToMove][square] & pawns & theirPieces){return _bitscanForward(lookupTables::pawnAttackTable[sideToMove][square] & pawns & theirPieces);}

    if(lookupTables::knightTable[square] & knights & theirPieces){return _bitscanForward(lookupTables::knightTable[square] & knights & theirPieces);}

    U64 bishopAttacks = lookupTables::getBishopAttacks(square, white | black) & theirPieces;
    if(bishopAttacks & bishops){return _bitscanForward(bishopAttacks & bishops);}

    U64 rookAttacks = lookupTables::getRookAttacks(square, white | black) & theirPieces;
    if(rookAttacks & rooks){return _bitscanForward(rookAttacks & rooks);}

    if((bishopAttacks | rookAttacks) & queens){return _bitscanForward((bishopAttacks | rookAttacks) & queens);}

    if(lookupTables::kingTable[square] & kings & theirPieces){return _bitscanForward(lookupTables::kingTable[square] & kings & theirPieces);}

    return 64;
  }

  //returns a bitboard with all pieces of a certain color attacking a certain square. Mainly used for Static Exchange Evaluation
  U64 squareAttackers(uint8_t square, Colors color){
    U64 pieces = getPieces(color);

    U64 bishopAttacks = lookupTables::getBishopAttacks(square, white | black) & pieces;
    U64 rookAttacks = lookupTables::getRookAttacks(square, white | black) & pieces;

    return (lookupTables::pawnAttackTable[sideToMove][square] & pawns & pieces)
          |(lookupTables::knightTable[square] & knights & pieces)
          |(bishopAttacks & bishops)
          |(rookAttacks & rooks)
          |((bishopAttacks | rookAttacks) & queens)
          |(lookupTables::kingTable[square] & kings & pieces); 
  }
  
  //squareUnderAttack() except we update canCurrentlyCastle
  bool kingUnderAttack(uint8_t square){
    bool result = squareUnderAttack(square) <= 63;

    if((square == sideToMove*56+3) && !result && (castlingRights & (sideToMove*6+2)) && (1ULL << square & ~occupied)){canCurrentlyCastle |= 2;}
    else if((square == sideToMove*56+5) && !result && (castlingRights & (sideToMove*3+1)) && (1ULL << square & ~occupied)){canCurrentlyCastle |= 1;}
    
    return result;
  }

  Pieces findPiece(uint8_t square){
    if(square>63){return null;}

    int pieces = mailbox[0][square];
    if(pieces >= 7){pieces -= 6;}

    return chess::Pieces(pieces);
  }

  //DON'T USE THIS; USE chess::makeMove() or search::makeMove() instead
  void makeMove(Move move){
    halfmoveClock++;
    const uint8_t startSquare = move.getStartSquare();
    const uint8_t endSquare = move.getEndSquare();
    const Pieces movingPiece = findPiece(startSquare);
    const MoveFlags moveFlags = move.getMoveFlags();

    mailbox[0][startSquare] = 0; mailbox[1][startSquare^56] = 0;
    unsetColors((1ULL << startSquare), sideToMove);
    unsetPieces(movingPiece, (1ULL << startSquare));

    if(moveFlags == ENPASSANT){
      U64 theirPawnSquare;
      if(sideToMove == WHITE){theirPawnSquare = (1ULL << endSquare) >> 8;}
      else{theirPawnSquare = (1ULL << endSquare) << 8;}
      uint8_t theirPawnSq = _bitscanForward(theirPawnSquare);
      mailbox[0][theirPawnSq] = 0; mailbox[1][theirPawnSq^56] = 0;
      unsetColors(theirPawnSquare, Colors(!sideToMove));
      unsetPieces(PAWN, theirPawnSquare);
    }
    else{
      if(getTheirPieces() & (1ULL << endSquare)){
        halfmoveClock = 0;
        startHistoryIndex = 0;
        mailbox[0][endSquare] = 0; mailbox[1][endSquare^56] = 0;
        unsetColors((1ULL << endSquare), Colors(!sideToMove));
        unsetPieces(UNKNOWN, (1ULL << endSquare));
      }
    }

    if(moveFlags == CASTLE){
      uint8_t rookStartSquare;
      uint8_t rookEndSquare;
      //Queenside Castling
      if(squareIndexToFile(endSquare) == 2){
        rookStartSquare = sideToMove*56;
        rookEndSquare = 3+sideToMove*56;
      }
      //Kingside Castling
      else{
        rookStartSquare = 7+sideToMove*56;
        rookEndSquare = 5+sideToMove*56;
      }
      mailbox[0][rookStartSquare] = 0; mailbox[1][rookStartSquare^56] = 0;
      unsetColors(rookStartSquare, sideToMove);
      unsetPieces(ROOK, rookStartSquare);

      mailbox[0][rookEndSquare] = sideToMove ? 10 : 4; mailbox[1][rookEndSquare^56] = sideToMove ? 4 : 10;
      setColors(rookEndSquare, sideToMove);
      setPieces(ROOK, rookEndSquare);
    }

    setColors((1ULL << endSquare), sideToMove);

    if(moveFlags == PROMOTION){
      mailbox[0][endSquare] = sideToMove ? move.getPromotionPiece()+6 : move.getPromotionPiece();
      mailbox[1][endSquare^56] = sideToMove ? move.getPromotionPiece() : move.getPromotionPiece()+6;
      setPieces(move.getPromotionPiece(), (1ULL << endSquare));
    }
    else{
      mailbox[0][endSquare] = sideToMove ? movingPiece+6 : movingPiece;
      mailbox[1][endSquare^56] = sideToMove ? movingPiece : movingPiece+6;
      setPieces(movingPiece, (1ULL << endSquare));
    }

    enPassant = 0ULL;
    if(movingPiece == PAWN){
      halfmoveClock = 0;
      startHistoryIndex = 0;
      enPassant = 0ULL;

      //double pawn push by white
      if((1ULL << endSquare) == (1ULL << startSquare) << 16){
        enPassant = (1ULL << startSquare) << 8;
      }
      //double pawn push by black
      else if((1ULL << endSquare) == (1ULL << startSquare) >> 16){
        enPassant = (1ULL << startSquare) >> 8;
      }
    }
    //Remove castling rights if king moved
    if(movingPiece == KING){
      if(sideToMove == WHITE){
        castlingRights &= ~(0x1 | 0x2);
      }
      else{
        castlingRights &= ~(0x4 | 0x8);
      }
    }
    //Remove castling rights if rook moved from starting square or if rook was captured
    if((startSquare == 0 && movingPiece == ROOK) || endSquare == 0){castlingRights &= ~0x2;}
    if((startSquare == 7 && movingPiece == ROOK) || endSquare == 7){castlingRights &= ~0x1;}
    if((startSquare == 56 && movingPiece == ROOK) || endSquare == 56){castlingRights &= ~0x8;}
    if((startSquare == 63 && movingPiece == ROOK) || endSquare == 63){castlingRights &= ~0x4;}
    
    occupied = white | black;
    sideToMove = Colors(!sideToMove);
  }
};


Move* generateLegalMoves(Board &board, Move* legalMoves){
  //Extremely useful source on how pointers/arrays work: https://cplusplus.com/doc/tutorial/pointers/
  Move* legalMovesPtr = legalMoves; //A pointer to the spot in memory where the next move will go
  uint8_t piecePos;

  KingMasks _kingMasks = board.generateKingMasks();

  U64 ourPieces = board.getOurPieces();
  U64 theirPieces = board.getTheirPieces();
  U64 notOurPieces = ~ourPieces;
  
  //Knight cannot move if it is pinned
  U64 pieceBitboard = (ourPieces & board.knights) & ~(_kingMasks.bishopPinnedPieces | _kingMasks.rookPinnedPieces);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);

    legalMovesPtr = MoveListFromBitboard(lookupTables::knightTable[piecePos] & notOurPieces & _kingMasks.checkmask, piecePos, false, legalMovesPtr);
  }

  pieceBitboard = (ourPieces & board.kings);
  assert(pieceBitboard);
  piecePos = _bitscanForward(pieceBitboard);
  board.canCurrentlyCastle = 0;

  U64 kingMovesBitboard = lookupTables::kingTable[piecePos] & notOurPieces;
  uint8_t endSquare;
  board.unsetColors(pieceBitboard, board.sideToMove); //when we check if the new position of king is under attack, we don't want the current king position to block the check
  while (kingMovesBitboard){
    endSquare = _popLsb(kingMovesBitboard);
    //check if new position of king is under attack
    if(!board.kingUnderAttack(endSquare)){*legalMovesPtr++ = Move(piecePos, endSquare);}
  }
  board.setColors(pieceBitboard, board.sideToMove); //revert the unsetColors call earlier
  //castling
  if(_kingMasks.checkmask == 0xFFFFFFFFFFFFFFFFULL){ //make sure king is not in check
    //Queenside castling
    if((board.canCurrentlyCastle & 0x2) && (1ULL << (board.sideToMove*56+2) & ~board.occupied) && (1ULL << (board.sideToMove*56+1) & ~board.occupied) && !(board.squareUnderAttack(board.sideToMove*56+2)<=63)){
      *legalMovesPtr++ = Move(piecePos, board.sideToMove*56+2, CASTLE);
    }
    //Kingside castling
    if((board.canCurrentlyCastle & 0x1) && (1ULL << (board.sideToMove*56+6) & ~board.occupied) && !(board.squareUnderAttack(board.sideToMove*56+6)<=63)){
      *legalMovesPtr++ = Move(piecePos, board.sideToMove*56+6, CASTLE);
    }
  }

  pieceBitboard = (ourPieces & board.pawns);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);

    if(1ULL << piecePos & (_kingMasks.bishopPinnedPieces | _kingMasks.rookPinnedPieces)){
      //if pawn is pinned, you cannot push it unless it is a vertical rook pin. 
      //Also, it is not possible to have squares directly in front of the pawn be part of a pinmask if the pawn is pinned unless it is a vertical rook pin
      //So just using the rook mask suffices
      if(!(1ULL << piecePos & _kingMasks.bishopPinnedPieces)){
        U64 singlePushBb = lookupTables::pawnPushTable[board.sideToMove][piecePos] & ~board.occupied;
        if(board.sideToMove == WHITE){legalMovesPtr = MoveListFromBitboard((singlePushBb | (singlePushBb << 8 & bitboards::rank4 & ~board.occupied)) & _kingMasks.checkmask & _kingMasks.rookPinmask, piecePos, true, legalMovesPtr);}
        else{legalMovesPtr = MoveListFromBitboard((singlePushBb | (singlePushBb >> 8 & bitboards::rank5 & ~board.occupied)) & _kingMasks.checkmask & _kingMasks.rookPinmask, piecePos, true, legalMovesPtr);}
      }

      //Using similar logic, using just the bishop mask suffices
      if(!(1ULL << piecePos & _kingMasks.rookPinnedPieces)){
        legalMovesPtr = MoveListFromBitboard(lookupTables::pawnAttackTable[board.sideToMove][piecePos] & theirPieces & _kingMasks.checkmask & _kingMasks.bishopPinmask, piecePos, true, legalMovesPtr);
      }
    }
    else{
      U64 singlePushBb = lookupTables::pawnPushTable[board.sideToMove][piecePos] & ~board.occupied;
      if(board.sideToMove == WHITE){legalMovesPtr = MoveListFromBitboard((singlePushBb | (singlePushBb << 8 & bitboards::rank4 & ~board.occupied)) & _kingMasks.checkmask, piecePos, PAWN, legalMovesPtr);}
      else{legalMovesPtr = MoveListFromBitboard((singlePushBb | (singlePushBb >> 8 & bitboards::rank5 & ~board.occupied)) & _kingMasks.checkmask, piecePos, true, legalMovesPtr);}

      legalMovesPtr = MoveListFromBitboard(lookupTables::pawnAttackTable[board.sideToMove][piecePos] & theirPieces & _kingMasks.checkmask, piecePos, true, legalMovesPtr);
    }

  }
  //With en passant, we can just test the move
  if(board.enPassant){
    uint8_t enPassantSquare = _bitscanForward(board.enPassant);
    U64 enPassantMovesBitboard = lookupTables::pawnAttackTable[!board.sideToMove][enPassantSquare] & (ourPieces & board.pawns);

    if(board.sideToMove == WHITE){board.unsetColors(board.enPassant >> 8, Colors(!board.sideToMove));}
    else{board.unsetColors(board.enPassant << 8, Colors(!board.sideToMove));}
    board.setColors(board.enPassant, board.sideToMove);
    while (enPassantMovesBitboard){
      uint8_t startSquare = _popLsb(enPassantMovesBitboard);
      board.unsetColors(1ULL << startSquare, board.sideToMove);
      //check if king is under attack
      if(!(board.squareUnderAttack(_bitscanForward(ourPieces & board.kings))<=63)){*legalMovesPtr++ = Move(startSquare, enPassantSquare, ENPASSANT);}

      board.setColors(1ULL << startSquare, board.sideToMove);
    }
    if(board.sideToMove == WHITE){board.setColors(board.enPassant >> 8, Colors(!board.sideToMove));} else{board.setColors(board.enPassant << 8, Colors(!board.sideToMove));}
    board.unsetColors(board.enPassant, board.sideToMove);
  }

  pieceBitboard = (ourPieces & board.rooks);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);
    //if rook is pinned by a bishop it cannot move, so we only check for if it is pinned by a rook
    if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){continue;}
    if(1ULL << piecePos & _kingMasks.rookPinnedPieces){
      legalMovesPtr = MoveListFromBitboard(lookupTables::getRookAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask & _kingMasks.rookPinmask, piecePos, false, legalMovesPtr);  
    }
    else{
      legalMovesPtr = MoveListFromBitboard(lookupTables::getRookAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask, piecePos, false, legalMovesPtr);     
    }
  }

  pieceBitboard = (ourPieces & board.bishops);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);

    //if bishop is pinned by a rook it cannot move, so we only check for if it is pinned by a bishop
    if(1ULL << piecePos & _kingMasks.rookPinnedPieces){continue;}
    if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){
      legalMovesPtr = MoveListFromBitboard(lookupTables::getBishopAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask & _kingMasks.bishopPinmask, piecePos, false, legalMovesPtr);    
    }
    else{
      legalMovesPtr = MoveListFromBitboard(lookupTables::getBishopAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask, piecePos, false, legalMovesPtr);   
    }
  }

  pieceBitboard = (ourPieces & board.queens);
  while(pieceBitboard){
    //for queen just treat it as a rook, then treat it as a bishop
    //as a rook:
    piecePos = _popLsb(pieceBitboard);

    //if rook is pinned by a bishop it cannot move, so we only check for if it is pinned by a rook
    if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){}
    else if(1ULL << piecePos & _kingMasks.rookPinnedPieces){
      legalMovesPtr = MoveListFromBitboard(lookupTables::getRookAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask & _kingMasks.rookPinmask, piecePos, false, legalMovesPtr); 
    }
    else{
      legalMovesPtr = MoveListFromBitboard(lookupTables::getRookAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask, piecePos, false, legalMovesPtr);   
    }

    //as a bishop:
    //if bishop is pinned by a rook it cannot move, so we only check for if it is pinned by a bishop
    if(1ULL << piecePos & _kingMasks.rookPinnedPieces){}
    else if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){
      legalMovesPtr = MoveListFromBitboard(lookupTables::getBishopAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask & _kingMasks.bishopPinmask, piecePos, false, legalMovesPtr);   
    }
    else{
      legalMovesPtr = MoveListFromBitboard(lookupTables::getBishopAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask, piecePos, false, legalMovesPtr); 
    }
  }

  return legalMovesPtr;
}

//Returns whether or not there is a legal move on the given board
//Used for stalemate and checkmate detection in calls for the getGameStatus() function below
bool isLegalMoves(Board& board){
  //Extremely useful source on how pointers/arrays work: https://cplusplus.com/doc/tutorial/pointers/
  uint8_t piecePos;

  KingMasks _kingMasks = board.generateKingMasks();

  U64 ourPieces = board.getOurPieces();
  U64 theirPieces = board.getTheirPieces();
  U64 notOurPieces = ~ourPieces;
  
  //Knight cannot move if it is pinned
  U64 pieceBitboard = (ourPieces & board.knights) & ~(_kingMasks.bishopPinnedPieces | _kingMasks.rookPinnedPieces);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);

    if(lookupTables::knightTable[piecePos] & notOurPieces & _kingMasks.checkmask){return true;}
  }

  pieceBitboard = (ourPieces & board.kings);
  piecePos = _bitscanForward(pieceBitboard);
  board.canCurrentlyCastle = 0;

  U64 kingMovesBitboard = lookupTables::kingTable[piecePos] & notOurPieces;
  uint8_t endSquare;
  board.unsetColors(pieceBitboard, board.sideToMove); //when we check if the new position of king is under attack, we don't want the current king position to block the check
  while (kingMovesBitboard){
    endSquare = _popLsb(kingMovesBitboard);
    //check if new position of king is under attack
    if(!board.kingUnderAttack(endSquare)){board.setColors(pieceBitboard, board.sideToMove); return true;}
  }
  board.setColors(pieceBitboard, board.sideToMove); //revert the unsetColors call earlier
  //castling
  if(_kingMasks.checkmask == 0xFFFFFFFFFFFFFFFFULL){ //make sure king is not in check
    //Queenside castling
    if((board.canCurrentlyCastle & 0x2) && (1ULL << (board.sideToMove*56+2) & ~board.occupied) && (1ULL << (board.sideToMove*56+1) & ~board.occupied) && !(board.squareUnderAttack(board.sideToMove*56+2)<=63)){
      return true;
    }
    //Kingside castling
    if((board.canCurrentlyCastle & 0x1) && (1ULL << (board.sideToMove*56+6) & ~board.occupied) && !(board.squareUnderAttack(board.sideToMove*56+6)<=63)){
      return true;
    }
  }


  pieceBitboard = (ourPieces & board.pawns);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);

    if(1ULL << piecePos & (_kingMasks.bishopPinnedPieces | _kingMasks.rookPinnedPieces)){
      //if pawn is pinned, you cannot push it unless it is a vertical rook pin. 
      //Also, it is not possible to have squares directly in front of the pawn be part of a pinmask if the pawn is pinned unless it is a vertical rook pin
      //So just using the rook mask suffices
      if(!(1ULL << piecePos & _kingMasks.bishopPinnedPieces)){
        U64 singlePushBb = lookupTables::pawnPushTable[board.sideToMove][piecePos] & ~board.occupied;
        if(board.sideToMove == WHITE){if((singlePushBb | (singlePushBb << 8 & bitboards::rank4 & ~board.occupied)) & _kingMasks.checkmask & _kingMasks.rookPinmask){return true;};}
        else{if((singlePushBb | (singlePushBb >> 8 & bitboards::rank5 & ~board.occupied)) & _kingMasks.checkmask & _kingMasks.rookPinmask){return true;};}
      }

      //Using similar logic, using just the bishop mask suffices
      if(!(1ULL << piecePos & _kingMasks.rookPinnedPieces)){
        if(lookupTables::pawnAttackTable[board.sideToMove][piecePos] & theirPieces & _kingMasks.checkmask & _kingMasks.bishopPinmask){return true;};
      }
    }
    else{
      U64 singlePushBb = lookupTables::pawnPushTable[board.sideToMove][piecePos] & ~board.occupied;
      if(board.sideToMove == WHITE){if((singlePushBb | (singlePushBb << 8 & bitboards::rank4 & ~board.occupied)) & _kingMasks.checkmask){return true;};}
      else{if((singlePushBb | (singlePushBb >> 8 & bitboards::rank5 & ~board.occupied)) & _kingMasks.checkmask){return true;};}

      if(lookupTables::pawnAttackTable[board.sideToMove][piecePos] & theirPieces & _kingMasks.checkmask){return true;};
    }
  }
  //With en passant, we can just test the move
  if(board.enPassant){
    uint8_t enPassantSquare = _bitscanForward(board.enPassant);
    U64 enPassantMovesBitboard = lookupTables::pawnAttackTable[!board.sideToMove][enPassantSquare] & (ourPieces & board.pawns);

    if(board.sideToMove == WHITE){board.unsetColors(board.enPassant >> 8, Colors(!board.sideToMove));}
    else{board.unsetColors(board.enPassant << 8, Colors(!board.sideToMove));}
    board.setColors(board.enPassant, board.sideToMove);
    while (enPassantMovesBitboard){
      uint8_t startSquare = _popLsb(enPassantMovesBitboard);
      board.unsetColors(1ULL << startSquare, board.sideToMove);
      //check if king is under attack
      if(!(board.squareUnderAttack(_bitscanForward(ourPieces & board.kings))<=63)){
        if(board.sideToMove == WHITE){board.setColors(board.enPassant >> 8, Colors(!board.sideToMove));} else{board.setColors(board.enPassant << 8, Colors(!board.sideToMove));}
        board.setColors(1ULL << startSquare, board.sideToMove);
        board.unsetColors(board.enPassant, board.sideToMove);
        return true;
      }
      board.setColors(1ULL << startSquare, board.sideToMove);
    }
    if(board.sideToMove == WHITE){board.setColors(board.enPassant >> 8, Colors(!board.sideToMove));} else{board.setColors(board.enPassant << 8, Colors(!board.sideToMove));}
    board.unsetColors(board.enPassant, board.sideToMove);
  }

  pieceBitboard = (ourPieces & board.rooks);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);
    //if rook is pinned by a bishop it cannot move, so we only check for if it is pinned by a rook
    if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){continue;}
    if(1ULL << piecePos & _kingMasks.rookPinnedPieces){
      if(lookupTables::getRookAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask & _kingMasks.rookPinmask){return true;};  
    }
    else{
      if(lookupTables::getRookAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask){return true;};     
    }
  }

  pieceBitboard = (ourPieces & board.bishops);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);

    //if bishop is pinned by a rook it cannot move, so we only check for if it is pinned by a bishop
    if(1ULL << piecePos & _kingMasks.rookPinnedPieces){continue;}
    if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){
      if(lookupTables::getBishopAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask & _kingMasks.bishopPinmask){return true;};    
    }
    else{
      if(lookupTables::getBishopAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask){return true;};   
    }
  }

  pieceBitboard = (ourPieces & board.queens);
  while(pieceBitboard){
    //for queen just treat it as a rook, then treat it as a bishop
    //as a rook:
    piecePos = _popLsb(pieceBitboard);

    //if rook is pinned by a bishop it cannot move, so we only check for if it is pinned by a rook
    if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){}
    else if(1ULL << piecePos & _kingMasks.rookPinnedPieces){
      if(lookupTables::getRookAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask & _kingMasks.rookPinmask){return true;}; 
    }
    else{
      if(lookupTables::getRookAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask){return true;};   
    }

    //as a bishop:
    //if bishop is pinned by a rook it cannot move, so we only check for if it is pinned by a bishop
    if(1ULL << piecePos & _kingMasks.rookPinnedPieces){}
    else if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){
      if(lookupTables::getBishopAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask & _kingMasks.bishopPinmask){return true;};   
    }
    else{
      if(lookupTables::getBishopAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask){return true;}; 
    }
  }

  return false;
}

struct MoveList{
  Move moveList[256]; //We assume that 256 is the maximum amount of moves in a position (it is what stockfish uses)
  Move* lastMove; //pointer to the last move in the moveList array

  MoveList(Board& board): lastMove(generateLegalMoves(board, moveList)){}

  MoveList(const MoveList& moves){
    std::copy(std::begin(moves.moveList), std::end(moves.moveList), std::begin(moveList));
    lastMove = moveList + moves.size();
  }

  Move* begin() {return moveList;}
  Move* end() {return lastMove;}

  Move operator[](int index) {return moveList[index];}

  size_t size() const {return lastMove-moveList;}
};

enum gameStatus{WIN = 1, DRAW = 0, LOSS = -1, ONGOING = 2};

gameStatus getGameStatus(Board& board, bool isLegalMoves){
  /*if(_popCount(board.occupied)<=5){
    auto tbProbeResult = tb_probe_wdl(board.white, board.black, board.kings, board.queens, board.rooks, board.bishops, board.knights, board.pawns, board.halfmoveClock, board.castlingRights, board.enPassant ? _bitscanForward(board.enPassant) : 0, board.sideToMove==WHITE);
    if(tbProbeResult==TB_RESULT_FAILED){board.printBoard(); assert(0);}
    if(tbProbeResult==TB_WIN){return WIN;}
    else if(tbProbeResult==TB_LOSS){return LOSS;}
    else{return DRAW;}
  }*/
  if(!isLegalMoves){
    //If our king is under attack, we lost from checkmate. Otherwise, it is a draw by stalemate.
    return gameStatus(-(board.squareUnderAttack(_bitscanForward(board.getOurPieces(KING)))<=63));
  }
  //Fifty Move Rule
  if(board.halfmoveClock>=100){return DRAW;}
  //Insufficient Material
  if(!(board.pawns | board.rooks | board.queens) &&//If there are pawns, rooks, or queens on the board, it is not insufficient material
  (_popCount(board.bishops | board.knights)<=1))
  {return DRAW;}
  //Threefold Repetition
  if(std::count(&board.history[board.startHistoryIndex], &board.history[board.halfmoveClock], board.history[board.halfmoveClock]) >= 2)
  {return DRAW;}

  return ONGOING;
}
}//namespace chess
//cutechess-cli -engine cmd=stockfish16 name=sf-tc20+0.2 tc=0/20+0.2 -engine cmd=stockfish16 name=sf-tc10+-.1 tc=0/10+0.1 -each proto=uci option.Threads=16 timemargin=1000 -rounds 100 -openings file=C:\Users\kjlji\Downloads\uho_2022\UHO_2022\UHO_2022_+150_+159\UHO_2022_8mvs_big_+140_+169.pgn plies=8 -draw movenumber=20 movecount=10 score=10 -resign movecount=10 score=1000 twosided=true -concurrency 2
//cutechess-cli -engine cmd=lc0 tc=0/60+6 option.WeightsFile=C:\Users\kjlji\Downloads\LeelaChessZeroDAG\lc0\t1-512x15x8h-distilled-swa-3395000.pb.gz -engine cmd=stockfish16 tc=0/10+1 -each proto=uci -rounds 10 -openings file=C:\Users\kjlji\Downloads\uho_2022\UHO_2022\UHO_2022_+150_+159\UHO_2022_8mvs_big_+140_+169.pgn plies=8 -debug