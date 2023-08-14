//Start reading code relating to move generation/chess rules in "bitboards.h"
//After reading this file, go to "uci.h"
#include <cstdint>
#include "lookup.h"
#include <sstream>
#include <vector>
#include <algorithm>

namespace chess{
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

  void printBoard(){
    U64 mask = 0;
    for(int i=8; i>0; i--){
      std::cout << "\n" << i << " ";
      for(int j=0; j<8; j++){
        mask = 0;
        mask |= (1ULL << (i-1)*8+j);
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
  
  //squareUnderAttack() except we update canCurrentlyCastle
  bool kingUnderAttack(uint8_t square){
    bool result = squareUnderAttack(square);

    if((square == sideToMove*56+3) && (castlingRights & sideToMove*6+2) && (1ULL << square & ~occupied) && !result){canCurrentlyCastle |= 2;}
    else if((square == sideToMove*56+5) && (castlingRights & sideToMove*3+1) && (1ULL << square & ~occupied) && !result){canCurrentlyCastle |= 1;}
    
    return result;
  }

  Pieces findPiece(uint8_t square){
    U64 startSquare = 1ULL << square;
    if(pawns & startSquare){return PAWN;}
    if(knights & startSquare){return KNIGHT;}
    if(bishops & startSquare){return BISHOP;}
    if(rooks & startSquare){return ROOK;}
    if(queens & startSquare){return QUEEN;}
    return KING;
  }

  void makeMove(Move move){
    halfmoveClock++;

    Pieces movingPiece = findPiece(move.getStartSquare());

    unsetColors((1ULL << move.getStartSquare()), sideToMove);
    unsetPieces(movingPiece, (1ULL << move.getStartSquare()));

    if(move.getMoveFlags() == ENPASSANT){
      U64 theirPawnSquare;
      if(sideToMove == WHITE){theirPawnSquare = (1ULL << move.getEndSquare()) >> 8;}
      else{theirPawnSquare = (1ULL << move.getEndSquare()) << 8;}
      unsetColors(theirPawnSquare, Colors(!sideToMove));
      unsetPieces(PAWN, theirPawnSquare);
    }
    else{
      halfmoveClock = 0;
      unsetColors((1ULL << move.getEndSquare()), Colors(!sideToMove));
      unsetPieces(UNKNOWN, (1ULL << move.getEndSquare()));
    }

    if(move.getMoveFlags() == CASTLE){
      uint8_t rookStartSquare;
      uint8_t rookEndSquare;
      //Queenside Castling
      if(move.getEndSquare() % 8 == 2){
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

    setColors((1ULL << move.getEndSquare()), sideToMove);
    if(move.getMoveFlags() == PROMOTION){setPieces(move.getPromotionPiece(), (1ULL << move.getEndSquare()));}
    else{setPieces(movingPiece, (1ULL << move.getEndSquare()));}

    enPassant = 0ULL;
    if(movingPiece == PAWN){
      halfmoveClock == 0;
      enPassant = 0ULL;

      //double pawn push by white
      if((1ULL << move.getEndSquare()) == (1ULL << move.getStartSquare()) << 16){
        enPassant = (1ULL << move.getStartSquare()) << 8;
      }
      //double pawn push by black
      else if((1ULL << move.getEndSquare()) == (1ULL << move.getStartSquare()) >> 16){
        enPassant = (1ULL << move.getStartSquare()) >> 8;
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
    if((move.getStartSquare() == 0 && movingPiece == ROOK) || move.getEndSquare() == 0){castlingRights &= ~0x2;}
    if((move.getStartSquare() == 7 && movingPiece == ROOK) || move.getEndSquare() == 7){castlingRights &= ~0x1;}
    if((move.getStartSquare() == 56 && movingPiece == ROOK) || move.getEndSquare() == 56){castlingRights &= ~0x8;}
    if((move.getStartSquare() == 63 && movingPiece == ROOK) || move.getEndSquare() == 63){castlingRights &= ~0x4;}
    
    occupied = white | black;
    sideToMove = Colors(!sideToMove);
  }
};

Move* generateLegalMoves(Board &board, Move* legalMoves){
  //Extremely useful source on how pointers/arrays work: https://cplusplus.com/doc/tutorial/pointers/
  Move* legalMovesPtr = legalMoves; //A pointer to the spot in memory where the next move will go
  uint8_t piecePos;

  KingMasks _kingMasks = board.generateKingMasks();

  U64 notOurPieces = ~board.getOurPieces();
  
  U64 pieceBitboard = board.getOurPieces(KNIGHT);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);

    //Knight cannot move if it is pinned
    if(!(1ULL << piecePos & (_kingMasks.bishopPinnedPieces | _kingMasks.rookPinnedPieces))){
      legalMovesPtr = MoveListFromBitboard(lookupTables::knightTable[piecePos] & notOurPieces & _kingMasks.checkmask, piecePos, KNIGHT, legalMovesPtr);
    }
  }

  pieceBitboard = board.getOurPieces(KING);
  piecePos = _bitscanForward(pieceBitboard);
  board.canCurrentlyCastle = 0;

  U64 kingMovesBitboard = lookupTables::kingTable[piecePos] & notOurPieces;
  uint8_t endSquare;
  board.unsetColors(pieceBitboard, board.sideToMove); //when we check if the new position of king is under attack, we don't want the current king position to block the check
  while (kingMovesBitboard){
    endSquare = _popLsb(kingMovesBitboard);
  
    //check if new position of king is under attack
    if(!board.kingUnderAttack(endSquare)){*(legalMovesPtr++) = Move(piecePos, endSquare);}
  }
  board.setColors(pieceBitboard, board.sideToMove); //revert the unsetColors call earlier
  //castling
  if(_kingMasks.checkmask == 0xFFFFFFFFFFFFFFFFULL){ //make sure king is not in check
    //Queenside castling
    if((board.canCurrentlyCastle & 0x2) && (1ULL << board.sideToMove*56+2 & ~board.occupied) && (1ULL << board.sideToMove*56+1 & ~board.occupied) && !board.squareUnderAttack(board.sideToMove*56+2)){
      *(legalMovesPtr++) = Move(piecePos, board.sideToMove*56+2, CASTLE);
    }
    //Kingside castling
    if((board.canCurrentlyCastle & 0x1) && (1ULL << board.sideToMove*56+6 & ~board.occupied) && !board.squareUnderAttack(board.sideToMove*56+6)){
      *(legalMovesPtr++) = Move(piecePos, board.sideToMove*56+6, CASTLE);
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
        if(board.sideToMove == WHITE){legalMovesPtr = MoveListFromBitboard((singlePushBb | (singlePushBb << 8 & bitboards::rank4 & ~board.occupied)) & _kingMasks.checkmask & _kingMasks.rookPinmask, piecePos, PAWN, legalMovesPtr);}
        else{legalMovesPtr = MoveListFromBitboard((singlePushBb | (singlePushBb >> 8 & bitboards::rank5 & ~board.occupied)) & _kingMasks.checkmask & _kingMasks.rookPinmask, piecePos, PAWN, legalMovesPtr);}
      }

      //Using similar logic, using just the bishop mask suffices
      if(!(1ULL << piecePos & _kingMasks.rookPinnedPieces)){
        legalMovesPtr = MoveListFromBitboard(lookupTables::pawnAttackTable[board.sideToMove][piecePos] & board.getTheirPieces() & _kingMasks.checkmask & _kingMasks.bishopPinmask, piecePos, PAWN, legalMovesPtr);
      }
    }
    else{
      U64 singlePushBb = lookupTables::pawnPushTable[board.sideToMove][piecePos] & ~board.occupied;
      if(board.sideToMove == WHITE){legalMovesPtr = MoveListFromBitboard((singlePushBb | (singlePushBb << 8 & bitboards::rank4 & ~board.occupied)) & _kingMasks.checkmask, piecePos, PAWN, legalMovesPtr);}
      else{legalMovesPtr = MoveListFromBitboard((singlePushBb | (singlePushBb >> 8 & bitboards::rank5 & ~board.occupied)) & _kingMasks.checkmask, piecePos, PAWN, legalMovesPtr);}

      legalMovesPtr = MoveListFromBitboard(lookupTables::pawnAttackTable[board.sideToMove][piecePos] & board.getTheirPieces() & _kingMasks.checkmask, piecePos, PAWN, legalMovesPtr);
    }

  }
  //With en passant, we can just test the move
  if(board.enPassant){
    uint8_t enPassantSquare = _bitscanForward(board.enPassant);
    U64 enPassantMovesBitboard = lookupTables::pawnAttackTable[!board.sideToMove][enPassantSquare] & board.getOurPieces(PAWN);

    board.unsetColors(1ULL << piecePos, board.sideToMove); 
    if(board.sideToMove == WHITE){board.unsetColors(board.enPassant >> 8, Colors(!board.sideToMove));} else{board.unsetColors(board.enPassant << 8, Colors(!board.sideToMove));}
    while (enPassantMovesBitboard){
      uint8_t startSquare = _popLsb(enPassantMovesBitboard);

      //check if king is under attack
      if(!board.squareUnderAttack(_bitscanForward(board.getOurPieces(KING)))){*(legalMovesPtr++) = Move(startSquare, enPassantSquare, ENPASSANT);}
    }
    board.setColors(1ULL << piecePos, board.sideToMove);
    if(board.sideToMove == WHITE){board.setColors(board.enPassant >> 8, Colors(!board.sideToMove));} else{board.setColors(board.enPassant << 8, Colors(!board.sideToMove));}
  }

  pieceBitboard = board.getOurPieces(ROOK);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);
    //if rook is pinned by a bishop it cannot move, so we only check for if it is pinned by a rook
    if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){continue;}
    if(1ULL << piecePos & _kingMasks.rookPinnedPieces){
      legalMovesPtr = MoveListFromBitboard(lookupTables::getRookAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask & _kingMasks.rookPinmask, piecePos, ROOK, legalMovesPtr);  
    }
    else{
      legalMovesPtr = MoveListFromBitboard(lookupTables::getRookAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask, piecePos, ROOK, legalMovesPtr);     
    }
  }

  pieceBitboard = board.getOurPieces(BISHOP);
  while(pieceBitboard){
    piecePos = _popLsb(pieceBitboard);

    //if bishop is pinned by a rook it cannot move, so we only check for if it is pinned by a bishop
    if(1ULL << piecePos & _kingMasks.rookPinnedPieces){continue;}
    if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){
      legalMovesPtr = MoveListFromBitboard(lookupTables::getBishopAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask & _kingMasks.bishopPinmask, piecePos, BISHOP, legalMovesPtr);    
    }
    else{
      legalMovesPtr = MoveListFromBitboard(lookupTables::getBishopAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask, piecePos, BISHOP, legalMovesPtr);   
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
      legalMovesPtr = MoveListFromBitboard(lookupTables::getRookAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask & _kingMasks.rookPinmask, piecePos, QUEEN, legalMovesPtr); 
    }
    else{
      legalMovesPtr = MoveListFromBitboard(lookupTables::getRookAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask, piecePos, QUEEN, legalMovesPtr);   
    }

    //as a bishop:
    //if bishop is pinned by a rook it cannot move, so we only check for if it is pinned by a bishop
    if(1ULL << piecePos & _kingMasks.rookPinnedPieces){}
    else if(1ULL << piecePos & _kingMasks.bishopPinnedPieces){
      legalMovesPtr = MoveListFromBitboard(lookupTables::getBishopAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask & _kingMasks.bishopPinmask, piecePos, QUEEN, legalMovesPtr);   
    }
    else{
      legalMovesPtr = MoveListFromBitboard(lookupTables::getBishopAttacks(piecePos, board.occupied) & notOurPieces & _kingMasks.checkmask, piecePos, QUEEN, legalMovesPtr); 
    }
  }

  return legalMovesPtr;
}

struct MoveList{
  Move moveList[256]; //We assume that 256 is the maximum amount of moves in a position (it is what stockfish uses)
  Move* lastMove;

  MoveList(Board board): lastMove(generateLegalMoves(board, moveList)){}

  Move* begin(){return moveList;}
  Move* end(){return lastMove;}

  int size(){return lastMove-moveList;}
};

}//namespace chess
//cutechess-cli -engine cmd=stockfish16 name=sf-tc20+0.2 tc=0/20+0.2 -engine cmd=stockfish16 name=sf-tc10+-.1 tc=0/10+0.1 -each proto=uci option.Threads=16 timemargin=1000 -rounds 100 -openings file=C:\Users\kjlji\Downloads\uho_2022\UHO_2022\UHO_2022_+150_+159\UHO_2022_8mvs_big_+140_+169.pgn plies=8 -draw movenumber=20 movecount=10 score=10 -resign movecount=10 score=1000 twosided=true -concurrency 2
//cutechess-cli -engine cmd=lc0 tc=0/60+6 option.WeightsFile=C:\Users\kjlji\Downloads\LeelaChessZeroDAG\lc0\t1-512x15x8h-distilled-swa-3395000.pb.gz -engine cmd=stockfish16 tc=0/10+1 -each proto=uci -rounds 10 -openings file=C:\Users\kjlji\Downloads\uho_2022\UHO_2022\UHO_2022_+150_+159\UHO_2022_8mvs_big_+140_+169.pgn plies=8 -debug