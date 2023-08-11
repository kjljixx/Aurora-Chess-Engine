//Start reading code relating to move generation/chess rules in "bitboards.h"
//After reading this file, go to "uci.h"
#include <cstdint>
#include "lookup.h"
#include <sstream>
#include <vector>
#include <algorithm>

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

struct Move{
  U64 startSquare;
  U64 endSquare;
  uint8_t startSquareIndex;
  uint8_t endSquareIndex;
  Pieces movedPiece;
  
  //If no capture or promotion, they are set to null
  Pieces capturedPiece;
  Pieces promotionPiece;

  bool isCastle;
  bool isEnPassant;

  Move(uint8_t startSquare, uint8_t endSquare, Pieces movedPiece, Pieces capturedPiece=UNKNOWN, Pieces promotionPiece=null, bool isCastle=false, bool isEnPassant=false):
    startSquare(1ULL << startSquare), startSquareIndex(startSquare),
    endSquare(1ULL << endSquare), endSquareIndex(endSquare),
    movedPiece(movedPiece),
    capturedPiece(capturedPiece), promotionPiece(promotionPiece), isCastle(isCastle), isEnPassant(isEnPassant) {}
  Move(U64 startSquare, U64 endSquare, Pieces movedPiece, Pieces capturedPiece=UNKNOWN, Pieces promotionPiece=null, bool isCastle=false, bool isEnPassant=false):
    startSquare(startSquare), startSquareIndex(_bitscanForward(startSquare)),
    endSquare(endSquare), endSquareIndex(_bitscanForward(endSquare)),
    movedPiece(movedPiece),
    capturedPiece(capturedPiece), promotionPiece(promotionPiece), isCastle(isCastle), isEnPassant(isEnPassant) {}
  //default constructor creates a null move (a1-a1)
  Move():
    startSquare(1ULL), startSquareIndex(0),
    endSquare(1ULL), endSquareIndex(0),
    capturedPiece(null), promotionPiece(null), isCastle(false), isEnPassant(false) {}
  
  //returns a string representation of the move (Pure Algebraic Coordinate Notation)
  std::string toStringRep(){
    std::string stringRep = squareIndexToNotation(startSquareIndex)+squareIndexToNotation(endSquareIndex);
    if(promotionPiece==KNIGHT){
      return stringRep+"n";
    }
    if(promotionPiece==QUEEN){
      return stringRep+"q";
    }
    if(promotionPiece==ROOK){
      return stringRep+"r";
    }
    if(promotionPiece==BISHOP){
      return stringRep+"b";
    }
    return stringRep;
  }
};

using MoveList = std::vector<Move>;

//Move flags like captured piece, castle, etc. need to be added separately.
MoveList MoveListFromBitboard(U64 moves, uint8_t startSquare, Pieces movedPiece, bool enPassant=false){
  MoveList movesList = {};
  while(moves){
    uint8_t endSquare = _popLsb(moves);
    if(movedPiece == PAWN && ((1ULL << endSquare) & 0xFF000000000000FFULL))//checks if move is pawn promotion. FF000000000000FF is the first and eight ranks
    {movesList.push_back(Move(startSquare, endSquare, movedPiece, UNKNOWN, KNIGHT, false, false));
    movesList.push_back(Move(startSquare, endSquare, movedPiece, UNKNOWN, BISHOP, false, false));
    movesList.push_back(Move(startSquare, endSquare, movedPiece, UNKNOWN, ROOK, false, false));
    movesList.push_back(Move(startSquare, endSquare, movedPiece, UNKNOWN, QUEEN, false, false));
    }
    else{
    movesList.push_back(Move(startSquare, endSquare, movedPiece, UNKNOWN, null, false, enPassant));
    }
  }
  return movesList;
}
MoveList MoveListFromBitboard(U64 moves, U64 startSquareBitboard, Pieces movedPiece, bool enPassant=false){
  uint8_t startSquare = _bitscanForward(startSquareBitboard);
  MoveList movesList = {};
  while(moves){
    movesList.push_back(Move(startSquare, _popLsb(moves), movedPiece, UNKNOWN, null, false, enPassant));
  }
  return movesList;
}

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
  Colors sideToMove = WHITE; unsigned char castlingRights; U64 enPassant; int halfmoveClock;

  //bitboards for all Pieces of each color
  U64 white;
  U64 black;

  //bitboard for all Pieces on the board
  U64 occupied;

  unsigned char canCurrentlyCastle;

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
  halfmoveClock(halfmoveClock)
  {
  }

  Board(){
    setToFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  }

  Board(std::string fen){
    setToFen(fen);
  }

  void setToFen(std::string fenString) {
    std::istringstream fenStream(fenString);
    std::string token;

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

    U64 boardPos = 56; // Fen string starts at a8 = index 56
    fenStream >> token;
    for (auto currChar : token) {
    switch (currChar) {
      case 'p': pawns |= (1ULL << boardPos); black |= (1ULL << boardPos++);
      break;
      case 'r': rooks |= (1ULL << boardPos); black |= (1ULL << boardPos++);
      break;
      case 'n': knights |= (1ULL << boardPos); black |= (1ULL << boardPos++);
      break;
      case 'b': bishops |= (1ULL << boardPos); black |= (1ULL << boardPos++);
      break;
      case 'q': queens |= (1ULL << boardPos); black |= (1ULL << boardPos++);
      break;
      case 'k': kings |= (1ULL << boardPos); black |= (1ULL << boardPos++);
      break;
      case 'P': pawns |= (1ULL << boardPos); white |= (1ULL << boardPos++);
      break;
      case 'R': rooks |= (1ULL << boardPos); white |= (1ULL << boardPos++);
      break;
      case 'N': knights |= (1ULL << boardPos); white |= (1ULL << boardPos++);
      break;
      case 'B': bishops |= (1ULL << boardPos); white |= (1ULL << boardPos++);
      break;
      case 'Q': queens |= (1ULL << boardPos); white |= (1ULL << boardPos++);
      break;
      case 'K': kings |= (1ULL << boardPos); white |= (1ULL << boardPos++);
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
      case UNKNOWN: U64 unsetBitboard = ~(1ULL << bit); pawns &= unsetBitboard; knights &= unsetBitboard; bishops &= unsetBitboard; rooks &= unsetBitboard; queens &= unsetBitboard; kings &= unsetBitboard; break;
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
      case UNKNOWN: U64 unsetBitboard = ~bit; pawns &= unsetBitboard; knights &= unsetBitboard; bishops &= unsetBitboard; rooks &= unsetBitboard; queens &= unsetBitboard; kings &= unsetBitboard; break;
    }
  }

  //generates checkmasks and pinmasks
  KingMasks generateKingMasks(){
    uint8_t square = _bitscanForward(getOurPieces(KING));
    U64 theirPieces = getTheirPieces();
    KingMasks _kingMasks;

    int numAttackers = 0;

    std::pair<int, int> squareRankFile = squareIndexToRankFile(square);
    if(lookupTables::knightTable[square] & knights & theirPieces){_kingMasks.checkmask |= lookupTables::knightTable[square] & knights & theirPieces; numAttackers++;}

    U64 rookAttacks = lookupTables::getRookAttacks(square, theirPieces) & (rooks | queens) & theirPieces;
    U64 attackRay;
    while(rookAttacks){
      int theirRook = _popLsb(rookAttacks);
      std::pair<int, int> theirRookRankFile = squareIndexToRankFile(theirRook);
      if(theirRookRankFile.first==squareRankFile.first){
        if(theirRookRankFile.second>squareRankFile.second){attackRay = rays::rays[2][square] & ~rays::rays[2][theirRook];}
        else{attackRay = rays::rays[3][square] & ~rays::rays[3][theirRook];}
      }
      else{
        if(theirRookRankFile.first>squareRankFile.first){attackRay = rays::rays[0][square] & ~rays::rays[0][theirRook];}
        else{attackRay = rays::rays[1][square] & ~rays::rays[1][theirRook];}
      }
      if(!(attackRay & getOurPieces())){_kingMasks.checkmask |= attackRay; numAttackers++;}
      else if(_popCount(attackRay & getOurPieces()) == 1){_kingMasks.rookPinmask |= attackRay; _kingMasks.rookPinnedPieces |= attackRay & getOurPieces();}
    }

    U64 bishopAttacks = lookupTables::getBishopAttacks(square, theirPieces) & (bishops | queens) & theirPieces;
    while(bishopAttacks){
      int theirBishop = _popLsb(bishopAttacks);
      std::pair<int, int> theirBishopRankFile = squareIndexToRankFile(theirBishop);
      if(theirBishopRankFile.first>squareRankFile.first){
        if(theirBishopRankFile.second<squareRankFile.second){attackRay = rays::rays[4][square] & ~rays::rays[4][theirBishop];}
        else{attackRay = rays::rays[5][square] & ~rays::rays[5][theirBishop];}
      }
      else{
        if(theirBishopRankFile.second<squareRankFile.second){attackRay = rays::rays[6][square] & ~rays::rays[6][theirBishop];}
        else{attackRay = rays::rays[7][square] & ~rays::rays[7][theirBishop];}
      }
      if(!(attackRay & getOurPieces())){_kingMasks.checkmask |= attackRay; numAttackers++;}
      else if(_popCount(attackRay & getOurPieces()) == 1){_kingMasks.bishopPinmask |= attackRay; _kingMasks.bishopPinnedPieces |= attackRay & getOurPieces();}
    }
    if(lookupTables::pawnAttackTable[sideToMove][square] & pawns & theirPieces){_kingMasks.checkmask |= lookupTables::pawnAttackTable[sideToMove][square] & pawns & theirPieces; numAttackers++;}
    
    if(numAttackers>1){_kingMasks.checkmask = 0ULL;}
    if(numAttackers==0){_kingMasks.checkmask = 0xFFFFFFFFFFFFFFFFULL;}
    return _kingMasks;
  }

  bool squareUnderAttack(uint8_t square){
    U64 theirPieces = getTheirPieces();
    //bitboards::printBoard(theirPieces);
    //std::cout << squareIndexToNotation(int(square));
    if(lookupTables::knightTable[square] & knights & theirPieces){return true;}
    if(lookupTables::getRookAttacks(square, white | black) & (rooks | queens) & theirPieces){return true;}
    if(lookupTables::getBishopAttacks(square, white | black) & (bishops | queens) & theirPieces){return true;}
    if(lookupTables::pawnAttackTable[sideToMove][square] & pawns & theirPieces){return true;}
    if(lookupTables::kingTable[square] & kings & theirPieces){return true;}

    return false;
  }
  
  //square under attack except we update canCurrentlyCastle
  bool kingUnderAttack(uint8_t square){
    bool result = squareUnderAttack(square);

    if((square == sideToMove*56+3) && (castlingRights & sideToMove*6+2) && (1ULL << square & ~occupied) && !result){canCurrentlyCastle |= 2;}
    else if((square == sideToMove*56+5) && (castlingRights & sideToMove*3+1) && (1ULL << square & ~occupied) && !result){canCurrentlyCastle |= 1;}
    
    return result;
  }

  void makeMove(Move move){
    halfmoveClock++;

    unsetColors(move.startSquare, sideToMove);
    unsetPieces(move.movedPiece, move.startSquare);
    //if(move.endSquareIndex == 42){std::cout << move.isEnPassant; std::cout << move.toStringRep();}
    if(move.isEnPassant){
      U64 theirPawnSquare;
      if(sideToMove == WHITE){theirPawnSquare = move.endSquare >> 8;}
      else{theirPawnSquare = move.endSquare << 8;}
      unsetColors(theirPawnSquare, Colors(!sideToMove));
      unsetPieces(PAWN, theirPawnSquare);
    }
    else if(move.capturedPiece!=null){
      halfmoveClock = 0;
      unsetColors(move.endSquare, Colors(!sideToMove));
      unsetPieces(move.capturedPiece, move.endSquare);
    }

    if(move.isCastle){
      uint8_t rookStartSquare;
      uint8_t rookEndSquare;
      //Queenside Castling
      if(move.endSquareIndex % 8 == 2){
        rookStartSquare = sideToMove*56;
        rookEndSquare = 3+sideToMove*56;
      }
      //Kingside Castling
      else{
        rookStartSquare = 7+sideToMove*56;
        rookEndSquare = 5+sideToMove*56;
      }
      unsetColors(rookStartSquare, sideToMove);
      unsetPieces(ROOK, rookStartSquare);

      setColors(rookEndSquare, sideToMove);
      setPieces(ROOK, rookEndSquare);
    }

    setColors(move.endSquare, sideToMove);
    if(move.promotionPiece == null){setPieces(move.movedPiece, move.endSquare);}
    else{setPieces(move.promotionPiece, move.endSquare);}

    enPassant = 0ULL;
    if(move.movedPiece == PAWN){
      halfmoveClock == 0;
      enPassant = 0ULL;

      //double pawn push by white
      if(move.endSquare == move.startSquare << 16){
        enPassant = move.startSquare << 8;
      }
      //double pawn push by black
      else if(move.endSquare == move.startSquare >> 16){
        enPassant = move.startSquare >> 8;
      }
    }
    //Remove castling rights if king moved
    if(move.movedPiece == KING){
      if(sideToMove == WHITE){
        castlingRights &= ~(0x1 | 0x2);
      }
      else{
        castlingRights &= ~(0x4 | 0x8);
      }
    }
    //Remove castling rights if rook moved from starting square or if rook was captured
    if((move.startSquareIndex == 0 && move.movedPiece == ROOK) || move.endSquareIndex == 0){castlingRights &= ~0x2;}
    if((move.startSquareIndex == 7 && move.movedPiece == ROOK) || move.endSquareIndex == 7){castlingRights &= ~0x1;}
    if((move.startSquareIndex == 56 && move.movedPiece == ROOK) || move.endSquareIndex == 56){castlingRights &= ~0x8;}
    if((move.startSquareIndex == 63 && move.movedPiece == ROOK) || move.endSquareIndex == 63){castlingRights &= ~0x4;}
    
    occupied = white | black;
    sideToMove = Colors(!sideToMove);
  }
};

//used for move generation to make sure we aren't moving Pieces of opponent color onto another piece of the same color
U64 opponentOrEmpty(Board &board){
  if(board.sideToMove == WHITE){return ~board.white;}
  return ~board.black;
}

MoveList generateLegalMoves(Board &board){
  MoveList legalMoves;
  MoveList moves;
  uint8_t piecePos;

  KingMasks _kingMasks = board.generateKingMasks();
  
  U64 pieceBitboard = board.getOurPieces(KNIGHT);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);

    //Knight cannot move if it is pinned
    if(!(1ULL << piecePos & (_kingMasks.bishopPinnedPieces | _kingMasks.rookPinnedPieces))){
      moves = MoveListFromBitboard(lookupTables::knightTable[piecePos] & opponentOrEmpty(board) & _kingMasks.checkmask, piecePos, KNIGHT);
      legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());
    }
  }

  pieceBitboard = board.getOurPieces(KING);
  piecePos = _bitscanForward(pieceBitboard);
  moves = MoveListFromBitboard(lookupTables::kingTable[piecePos] & opponentOrEmpty(board), piecePos, KING);
  //check if new position of king is under attack
  board.unsetColors(pieceBitboard, board.sideToMove);
  moves.erase(std::remove_if(moves.begin(), moves.end(), [&](Move m){return board.kingUnderAttack(m.endSquareIndex);}), moves.end());
  board.setColors(pieceBitboard, board.sideToMove);
  legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());

  if(_kingMasks.checkmask == 0xFFFFFFFFFFFFFFFFULL){ //make sure king is not in check
    //Queenside castling
    if((board.canCurrentlyCastle & 0x2) && (1ULL << board.sideToMove*56+2 & ~board.occupied) && (1ULL << board.sideToMove*56+1 & ~board.occupied) && !board.squareUnderAttack(board.sideToMove*56+2)){
      legalMoves.push_back(Move(piecePos, board.sideToMove*56+2, KING, null, null, true, false));
    }
    //Kingside castling
    if((board.canCurrentlyCastle & 0x1) && (1ULL << board.sideToMove*56+6 & ~board.occupied) && !board.squareUnderAttack(board.sideToMove*56+6)){
      legalMoves.push_back(Move(piecePos, board.sideToMove*56+6, KING, null, null, true, false));
    }
  }

  pieceBitboard = board.getOurPieces(PAWN);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);

    if(1ULL << piecePos & (_kingMasks.bishopPinnedPieces | _kingMasks.rookPinnedPieces)){
      //if pawn is pinned, you cannot push it unless it is a vertical rook pin. 
      //Also, it is not possible to have squares directly in front of the pawn be part of a pinmask if the pawn is pinned unless it is a vertical rook pin
      //So just using the rook mask suffices
      if(!(1ULL << piecePos & _kingMasks.bishopPinnedPieces)){
        U64 singlePushBb = lookupTables::pawnPushTable[board.sideToMove][piecePos] & ~board.occupied;
        if(board.sideToMove == WHITE){moves = MoveListFromBitboard((singlePushBb | (singlePushBb << 8 & bitboards::rank4 & ~board.occupied)) & _kingMasks.checkmask & _kingMasks.rookPinmask, piecePos, PAWN);}
        else{moves = MoveListFromBitboard((singlePushBb | (singlePushBb >> 8 & bitboards::rank5 & ~board.occupied)) & _kingMasks.checkmask & _kingMasks.rookPinmask, piecePos, PAWN);}
        legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());
      }

      //Using similar logic, using just the bishop mask suffices
      if(!(1ULL << piecePos & _kingMasks.rookPinnedPieces)){
        moves = MoveListFromBitboard(lookupTables::pawnAttackTable[board.sideToMove][piecePos] & board.getTheirPieces() & _kingMasks.checkmask & _kingMasks.bishopPinmask, piecePos, PAWN);
        legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());
      }
    }
    else{
      U64 singlePushBb = lookupTables::pawnPushTable[board.sideToMove][piecePos] & ~board.occupied;
      if(board.sideToMove == WHITE){moves = MoveListFromBitboard((singlePushBb | (singlePushBb << 8 & bitboards::rank4 & ~board.occupied)) & _kingMasks.checkmask, piecePos, PAWN);}
      else{moves = MoveListFromBitboard((singlePushBb | (singlePushBb >> 8 & bitboards::rank5 & ~board.occupied)) & _kingMasks.checkmask, piecePos, PAWN);}
      legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());

      moves = MoveListFromBitboard(lookupTables::pawnAttackTable[board.sideToMove][piecePos] & board.getTheirPieces() & _kingMasks.checkmask, piecePos, PAWN);
      legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());
    }

    //With en passant, we can just test the move
    if(board.enPassant){
      moves = MoveListFromBitboard(lookupTables::pawnAttackTable[board.sideToMove][piecePos] & board.enPassant, piecePos, PAWN, true);
      if(moves.size()>0){
        board.unsetColors(1ULL << piecePos, board.sideToMove); 
        if(board.sideToMove == WHITE){board.unsetColors(board.enPassant >> 8, Colors(!board.sideToMove));} else{board.unsetColors(board.enPassant << 8, Colors(!board.sideToMove));}
        if(!board.squareUnderAttack(_bitscanForward(board.getOurPieces(KING)))){
          legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());
        }
        board.setColors(1ULL << piecePos, board.sideToMove);
        if(board.sideToMove == WHITE){board.setColors(board.enPassant >> 8, Colors(!board.sideToMove));} else{board.setColors(board.enPassant << 8, Colors(!board.sideToMove));}
      }
    }
  }

  pieceBitboard = board.getOurPieces(ROOK);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);
    //if rook is pinned by a bishop it cannot move, so we only check for if it is pinned by a rook
    if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){continue;}
    if(1ULL << piecePos & _kingMasks.rookPinnedPieces){
      moves = MoveListFromBitboard(lookupTables::getRookAttacks(piecePos, board.occupied) & opponentOrEmpty(board) & _kingMasks.checkmask & _kingMasks.rookPinmask, piecePos, ROOK);
      legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());      
    }
    else{
      moves = MoveListFromBitboard(lookupTables::getRookAttacks(piecePos, board.occupied) & opponentOrEmpty(board) & _kingMasks.checkmask, piecePos, ROOK);
      legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());      
    }
  }

  pieceBitboard = board.getOurPieces(BISHOP);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);

    //if bishop is pinned by a rook it cannot move, so we only check for if it is pinned by a bishop
    if(1ULL << piecePos & _kingMasks.rookPinnedPieces){continue;}
    if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){
      moves = MoveListFromBitboard(lookupTables::getBishopAttacks(piecePos, board.occupied) & opponentOrEmpty(board) & _kingMasks.checkmask & _kingMasks.bishopPinmask, piecePos, BISHOP);
      legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());      
    }
    else{
      moves = MoveListFromBitboard(lookupTables::getBishopAttacks(piecePos, board.occupied) & opponentOrEmpty(board) & _kingMasks.checkmask, piecePos, BISHOP);
      legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());      
    }
  }

  pieceBitboard = board.getOurPieces(QUEEN);
  while(pieceBitboard){
    //for queen just treat it as a rook, then treat it as a bishop
    //as a rook:
    piecePos = _popLsb(pieceBitboard);

    //if rook is pinned by a bishop it cannot move, so we only check for if it is pinned by a rook
    if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){}
    else if(1ULL << piecePos & _kingMasks.rookPinnedPieces){
      moves = MoveListFromBitboard(lookupTables::getRookAttacks(piecePos, board.occupied) & opponentOrEmpty(board) & _kingMasks.checkmask & _kingMasks.rookPinmask, piecePos, QUEEN);
      legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());      
    }
    else{
      moves = MoveListFromBitboard(lookupTables::getRookAttacks(piecePos, board.occupied) & opponentOrEmpty(board) & _kingMasks.checkmask, piecePos, QUEEN);
      legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());      
    }

    //as a bishop:
    //if bishop is pinned by a rook it cannot move, so we only check for if it is pinned by a bishop
    if(1ULL << piecePos & _kingMasks.rookPinnedPieces){}
    else if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){
      moves = MoveListFromBitboard(lookupTables::getBishopAttacks(piecePos, board.occupied) & opponentOrEmpty(board) & _kingMasks.checkmask & _kingMasks.bishopPinmask, piecePos, QUEEN);
      legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());      
    }
    else{
      moves = MoveListFromBitboard(lookupTables::getBishopAttacks(piecePos, board.occupied) & opponentOrEmpty(board) & _kingMasks.checkmask, piecePos, QUEEN);
      legalMoves.insert(legalMoves.end(), moves.begin(), moves.end());      
    }
  }

  return legalMoves;
}
}//namespace chess
//cutechess-cli -engine cmd=stockfish16 name=sf-tc20+0.2 tc=0/20+0.2 -engine cmd=stockfish16 name=sf-tc10+-.1 tc=0/10+0.1 -each proto=uci option.Threads=16 timemargin=1000 -rounds 100 -openings file=C:\Users\kjlji\Downloads\uho_2022\UHO_2022\UHO_2022_+150_+159\UHO_2022_8mvs_big_+140_+169.pgn plies=8 -draw movenumber=20 movecount=10 score=10 -resign movecount=10 score=1000 twosided=true -concurrency 2
//cutechess-cli -engine cmd=lc0 tc=0/60+6 option.WeightsFile=C:\Users\kjlji\Downloads\LeelaChessZeroDAG\lc0\t1-512x15x8h-distilled-swa-3395000.pb.gz -engine cmd=stockfish16 tc=0/10+1 -each proto=uci -rounds 10 -openings file=C:\Users\kjlji\Downloads\uho_2022\UHO_2022\UHO_2022_+150_+159\UHO_2022_8mvs_big_+140_+169.pgn plies=8 -debug