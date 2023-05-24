#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <chrono>

int isInCheckCalls = 0;
std::vector<int> times;

namespace chess{
//all piece related lists/ints use 0 as pawns, 1 as knights, 2 as bishops, 3 as rooks, 4 as queens, and 5 as kings
//all board related lists/ints/pairs come in the form (y, x) where (0, 0) is a8, (1, 0) is a a7, (0, 1) is b8, etc.
std::vector<std::vector<std::vector<std::vector<int>>>> pawnMoves;
std::vector<std::vector<std::vector<int>>> knightMoves;
std::vector<std::vector<std::vector<int>>> bishopMoves;
std::vector<std::vector<std::vector<int>>> rookMoves;
std::vector<std::vector<std::vector<int>>> queenMoves;
std::vector<std::vector<std::vector<int>>> kingMoves;


class move{
    public:
    std::pair<int, int> startCoords;
    int movedPieceType;
    int movedPieceIndex;
    std::pair<int, int> endCoords;
    bool isEnPassant;
    int capturedPiece; //-1 if no capture
    int originalFiftyMoveCount;
    std::pair<int, int> originalEnPassantSquare;
    std::vector<std::pair<bool, bool>> originalCastling;
};

class board{
    public:
    //DEBUG
    bool outputMoves = true;
    // fullBoard[0][0] is a8, fullboard[1][0] is a7, fullboard[0][1] is b8
    std::vector<std::vector<std::vector<int>>> fullBoard; 
    //each vector contains pairs of 0-indexed coordinates of all the pieces of the specified type
    std::vector<std::vector<std::vector<std::pair<int, int>>>> allPieces;
    //std::vector<board> history;

    bool canEnPassant;
    std::pair<int, int> enPassantSquare;

    std::vector<std::pair<bool, bool>> castling; // [[wA, wH], [bA, bH]]

    int sideToMove; // 0 if white to move, 1 if black to move

    int fiftyMoveRuleCounter;

    //initializes board, arguments: fen string, default is starting position
    void initialize(std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"){
        kingMoves.resize(10, std::vector<std::vector<int>>(2, std::vector<int>(2, 8))); //8 is a filler for extra moves
        int it=0; //iterates through types of moves
        for(int i=-1; i<2; i++){
            for(int j=-1; j<2; j++){
                if(i!=0 || j!=0){
                    kingMoves[it][0][0] = i;
                    kingMoves[it][0][1] = j;
                    it++;
                }
                if(i==0 && j!=0){
                    it--;
                    kingMoves[it][1][0] = 0;
                    kingMoves[it][1][1] = j*2;
                    it++;
                }
            }
        }
        queenMoves.resize(8, std::vector<std::vector<int>>(7, std::vector<int>(2)));
        it=0; //iterates through types of moves
        for(int i=-1; i<2; i++){
            for(int j=-1; j<2; j++){
                if(i!=0 || j!=0){
                    for(int k=1; k<8; k++){
                        queenMoves[it][k-1][0] = i*k;
                        queenMoves[it][k-1][1] = j*k;
                    }
                    it++;
                }
            }
        }
        rookMoves.resize(4, std::vector<std::vector<int>>(7, std::vector<int>(2)));
        it=0; //iterates through types of moves
        for(int i=-1; i<2; i++){
            for(int j=-1; j<2; j++){
                if((i==0 || j==0)&&(i!=0 || j!=0)){
                    for(int k=1; k<8; k++){
                        rookMoves[it][k-1][0] = i*k;
                        rookMoves[it][k-1][1] = j*k;
                    }
                    it++;
                }
            }
        }
        bishopMoves.resize(4, std::vector<std::vector<int>>(7, std::vector<int>(2)));
        it=0; //iterates through types of moves
        for(int i=-1; i<2; i++){
            for(int j=-1; j<2; j++){
                if(i!=0 && j!=0){
                    for(int k=1; k<8; k++){
                        bishopMoves[it][k-1][0] = i*k;
                        bishopMoves[it][k-1][1] = j*k;
                    }
                    it++;
                }
            }
        }
        knightMoves.resize(8, std::vector<std::vector<int>>(1, std::vector<int>(2)));
        it=0; //iterates through types of moves
        for(int i=-1; i<2; i++){
            for(int j=-1; j<2; j++){
                if(i!=0 && j!=0){
                    for(int k=1; k<3; k++){
                        knightMoves[it][0][0] = i*k;
                        knightMoves[it][0][1] = j*(3-k);
                        it++;
                    }
                }
            }
        }
        pawnMoves.resize(2, std::vector<std::vector<std::vector<int>>>(3, std::vector<std::vector<int>>(2, std::vector<int>(2, 8)))); //3 "types of moves", push, capture left, capture right. since possible pawn moves differ for each side, we have a dfferent vector of moves for each side
        for(int s=0; s<2; s++){
            it=0; //iterates through types of moves
            pawnMoves[s][it][0][0] = 1*(s*2-1); pawnMoves[s][it][0][1] = 0;
            pawnMoves[s][it][1][0] = 2*(s*2-1); pawnMoves[s][it][1][1] = 0;
            it++;
            pawnMoves[s][it][0][0] = 1*(s*2-1); pawnMoves[s][it][0][1] = -1;
            it++;
            pawnMoves[s][it][0][0] = 1*(s*2-1); pawnMoves[s][it][0][1] = 1;
        }
        int y = 0;
        int x = 0;
        fullBoard.resize(8, std::vector<std::vector<int>>(8, std::vector<int>(2, -1)));
        //std::cout << fullBoard.size();
        allPieces.resize(6, std::vector<std::vector<std::pair<int, int>>>(2));
        castling.resize(2, std::make_pair(false, false));
        for(int i=0; i<fen.size(); i++){
            //std::cout << " " <<"|x:" << x << "|y:" << y << "|";
            if(!(x==8 && y==7)){
                if(std::isdigit(fen[i])){
                    x+=(int)fen[i]-48;
                }
                else if(fen[i]==47){
                    y++;
                    x = 0;
                }
                else{
                    //std::cout << fen[i] << " ";
                    if(fen[i]=='p'){allPieces[0][1].push_back(std::make_pair(y, x)); fullBoard[y][x][0]=1; fullBoard[y][x][1]=0;}
                    if(fen[i]=='n'){allPieces[1][1].push_back(std::make_pair(y, x)); fullBoard[y][x][0]=1; fullBoard[y][x][1]=1;}
                    if(fen[i]=='b'){allPieces[2][1].push_back(std::make_pair(y, x)); fullBoard[y][x][0]=1; fullBoard[y][x][1]=2;}
                    if(fen[i]=='r'){allPieces[3][1].push_back(std::make_pair(y, x)); fullBoard[y][x][0]=1; fullBoard[y][x][1]=3;}
                    if(fen[i]=='q'){allPieces[4][1].push_back(std::make_pair(y, x)); fullBoard[y][x][0]=1; fullBoard[y][x][1]=4;}
                    if(fen[i]=='k'){allPieces[5][1].push_back(std::make_pair(y, x)); fullBoard[y][x][0]=1; fullBoard[y][x][1]=5;}
                    if(fen[i]=='P'){allPieces[0][0].push_back(std::make_pair(y, x)); fullBoard[y][x][0]=0; fullBoard[y][x][1]=0;}
                    if(fen[i]=='N'){allPieces[1][0].push_back(std::make_pair(y, x)); fullBoard[y][x][0]=0; fullBoard[y][x][1]=1;}
                    if(fen[i]=='B'){allPieces[2][0].push_back(std::make_pair(y, x)); fullBoard[y][x][0]=0; fullBoard[y][x][1]=2;}
                    if(fen[i]=='R'){allPieces[3][0].push_back(std::make_pair(y, x)); fullBoard[y][x][0]=0; fullBoard[y][x][1]=3;}
                    if(fen[i]=='Q'){allPieces[4][0].push_back(std::make_pair(y, x)); fullBoard[y][x][0]=0; fullBoard[y][x][1]=4;}
                    if(fen[i]=='K'){allPieces[5][0].push_back(std::make_pair(y, x)); fullBoard[y][x][0]=0; fullBoard[y][x][1]=5;}
                    x++;
                }
            }
            else{
                i++;
                if(fen[i]=='w'){sideToMove=0;}
                else{sideToMove=1;}
                i+=2;
                for(int j=0; j<4; j++){
                    if(fen[i]==' '){
                        //std::cout << i;
                        //std::cout << "[" << fen[i] << "]";
                        break;
                    }
                    else if(fen[i]=='K' || fen[i]=='H'){castling[0].second = true;}
                    else if(fen[i]=='Q' || fen[i]=='A'){castling[0].first = true;}
                    else if(fen[i]=='k' || fen[i]=='h'){castling[1].second = true;}
                    else if(fen[i]=='q' || fen[i]=='a'){castling[1].first = true;}
                    //std::cout << "<";
                    i++;
                }
                i++;
                //std::cout << i;
                //std::cout << "["<< fen[i] << "]";
                if(fen[i]=='-'){
                    ////std::cout << "no en passant";
                    canEnPassant = false;
                }
                else{
                    canEnPassant = true;
                    enPassantSquare = std::make_pair(8-(fen[i+1]-48), fen[i]-97);
                    std::cout << enPassantSquare.second;
                    i++;
                }
                //std::cout << "[" << fen[i] << "]";
                i+=2;
                std::string s;
                //std::cout << std::endl;
                //std::cout << i;
                //std::cout << std::endl;
                while(fen[i]!=' ' && i<fen.size()){
                    s += fen[i];
                    //std::cout << fen[i];
                    i++;
                }
                //std::cout << std::endl;
                //std::cout << s;
                fiftyMoveRuleCounter = std::stoi(s);
                break;
            }
        }
        //std::cout << "DEBUG VALUES:" << fullBoard[5][0][0];
    }

    bool isInCheck(){
        isInCheckCalls++;
        int x;
        int y;
        //horizontal checks
        y = allPieces[5][sideToMove][0].first;
        for(int x=allPieces[5][sideToMove][0].second+1; x<8; x++){
            if(fullBoard[y][x][0]==!sideToMove && (fullBoard[y][x][1]==3 || fullBoard[y][x][1]==4)){
                return true;
            }
            if(fullBoard[y][x][0]!=-1){
                break;
            }
        }
        for(int x=allPieces[5][sideToMove][0].second-1; x>=0; x--){
            if(fullBoard[y][x][0]==!sideToMove && (fullBoard[y][x][1]==3 || fullBoard[y][x][1]==4)){
                return true;
            }
            if(fullBoard[y][x][0]!=-1){
                break;
            }
        }
        x = allPieces[5][sideToMove][0].second;
        for(int y=allPieces[5][sideToMove][0].first+1; y<8; y++){
            if(fullBoard[y][x][0]==!sideToMove && (fullBoard[y][x][1]==3 || fullBoard[y][x][1]==4)){
                return true;
            }
            if(fullBoard[y][x][0]!=-1){
                break;
            }
        }
        for(int y=allPieces[5][sideToMove][0].first-1; y>=0; y--){
            if(fullBoard[y][x][0]==!sideToMove && (fullBoard[y][x][1]==3 || fullBoard[y][x][1]==4)){
                return true;
            }
            if(fullBoard[y][x][0]!=-1){
                break;
            }
        }
        //diagonal checks
        y = allPieces[5][sideToMove][0].first-1;
        if(y>=0 && y<=7){
            for(int x=allPieces[5][sideToMove][0].second+1; x<8; x++){
                if(fullBoard[y][x][0]==!sideToMove && (fullBoard[y][x][1]==2 || fullBoard[y][x][1]==4)){
                    return true;
                }
                if(fullBoard[y][x][0]!=-1){
                    break;
                }
                y--;
                if(y<0){
                    break;
                }
            }
        }
        y = allPieces[5][sideToMove][0].first-1;
        if(y>=0 && y<=7){
            for(int x=allPieces[5][sideToMove][0].second-1; x>=0; x--){
                //std::cout <<x << "\n";
                if(fullBoard[y][x][0]==!sideToMove && (fullBoard[y][x][1]==2 || fullBoard[y][x][1]==4)){
                    return true;
                }
                if(fullBoard[y][x][0]!=-1){
                    break;
                }
                y--;
                if(y<0){
                    break;
                }
            }
        }
        y = allPieces[5][sideToMove][0].first+1;
        if(y>=0 && y<=7){
            for(int x=allPieces[5][sideToMove][0].second+1; x<8; x++){
                if(fullBoard[y][x][0]==!sideToMove && (fullBoard[y][x][1]==2 || fullBoard[y][x][1]==4)){
                    return true;
                }
                if(fullBoard[y][x][0]!=-1){
                    break;
                }
                y++;
                if(y>7){
                    break;
                }
            }
        }
        y = allPieces[5][sideToMove][0].first+1;
        if(y>=0 && y<=7){
            for(int x=allPieces[5][sideToMove][0].second-1; x>=0; x--){
                if(fullBoard[y][x][0]==!sideToMove && (fullBoard[y][x][1]==2 || fullBoard[y][x][1]==4)){
                    return true;
                }
                if(fullBoard[y][x][0]!=-1){
                    break;
                }
                y++;
                if(y>7){
                    break;
                }
            }
        }
        //pawn checks
        x = allPieces[5][sideToMove][0].second-1;
        if(x>=0 && allPieces[5][sideToMove][0].first!=7 && allPieces[5][sideToMove][0].first!=0){
            if(fullBoard[allPieces[5][sideToMove][0].first+(sideToMove*2-1)][x][0]==!sideToMove && fullBoard[allPieces[5][sideToMove][0].first+(sideToMove*2-1)][x][1]==0){
                return true;
            }
        }
        x+=2;
        if(x<8 && allPieces[5][sideToMove][0].first!=7 && allPieces[5][sideToMove][0].first!=0){
            if(fullBoard[allPieces[5][sideToMove][0].first+(sideToMove*2-1)][x][0]==!sideToMove && fullBoard[allPieces[5][sideToMove][0].first+(sideToMove*2-1)][x][1]==0){
                return true;
            }
        }
        //knight checks
        x = allPieces[5][sideToMove][0].second-1;
        y = allPieces[5][sideToMove][0].first-2;
        //std::cout << allPieces[5][sideToMove][0].first;
        if(x>=0 && y>=0){
            if(fullBoard[y][x][0]==!sideToMove && fullBoard[y][x][1]==1){
                //std::cout << "KNIGHT0CHECK";
                return true;
            }
        }
        x+=2;
        if(x<8 && y>=0){
            if(fullBoard[y][x][0]==!sideToMove && fullBoard[y][x][1]==1){
                //std::cout << "KNIGHT1CHECK";
                return true;
            }
        }
        y+=4;
        if(x<8 && y<8){
            if(fullBoard[y][x][0]==!sideToMove && fullBoard[y][x][1]==1){
                //std::cout << "KNIGHT2CHECK";
                return true;
            }
        }
        x-=2;
        if(x>=0 && y<8){
            if(fullBoard[y][x][0]==!sideToMove && fullBoard[y][x][1]==1){
                //std::cout << "KNIGHT"<< y<< "CHECK";
                return true;
            }
        }
        x = allPieces[5][sideToMove][0].second-2;
        y = allPieces[5][sideToMove][0].first-1;
        //std::cout << allPieces[5][sideToMove][0].first;
        if(x>=0 && y>=0){
            if(fullBoard[y][x][0]==!sideToMove && fullBoard[y][x][1]==1){
                //std::cout << "KNIGHT0CHECK";
                return true;
            }
        }
        x+=4;
        if(x<8 && y>=0){
            if(fullBoard[y][x][0]==!sideToMove && fullBoard[y][x][1]==1){
                //std::cout << "KNIGHT1CHECK";
                return true;
            }
        }
        y+=2;
        if(x<8 && y<8){
            if(fullBoard[y][x][0]==!sideToMove && fullBoard[y][x][1]==1){
                //std::cout << "KNIGHT2CHECK";
                return true;
            }
        }
        x-=4;
        if(x>=0 && y<8){
            if(fullBoard[y][x][0]==!sideToMove && fullBoard[y][x][1]==1){
                //std::cout << "KNIGHT"<< y<< "CHECK";
                return true;
            }
        }
        return false;
    }

    int makeMove(int piece, int index, std::pair<int, int> endCoords, bool isEnPassant){ //internal makeMove which doesnt update side to move, fifty move rule, castling, etc.
        int capturedPiece = -1;
        if(fullBoard[endCoords.first][endCoords.second][0]!=-1){
            capturedPiece = fullBoard[endCoords.first][endCoords.second][1];
            //std::cout << "[" << fullBoard[endCoords.first][endCoords.second][0] << "]";
            for(int i=0; i<allPieces[fullBoard[endCoords.first][endCoords.second][1]][fullBoard[endCoords.first][endCoords.second][0]].size(); i++){
                if(allPieces[fullBoard[endCoords.first][endCoords.second][1]][fullBoard[endCoords.first][endCoords.second][0]][i]==endCoords){
                    auto it = allPieces[fullBoard[endCoords.first][endCoords.second][1]][fullBoard[endCoords.first][endCoords.second][0]].begin()+i;
                    allPieces[fullBoard[endCoords.first][endCoords.second][1]][fullBoard[endCoords.first][endCoords.second][0]].erase(it);
                    break;
                }
            }
        }
        if(endCoords.first==2 || endCoords.first==5){
            if(fullBoard[endCoords.first+sideToMove*-2+1][endCoords.second][0]!=-1 && isEnPassant){
                for(int i=0; i<allPieces[fullBoard[endCoords.first+sideToMove*-2+1][endCoords.second][1]][fullBoard[endCoords.first+sideToMove*-2+1][endCoords.second][0]].size(); i++){
                    if(allPieces[fullBoard[endCoords.first+(sideToMove*-2+1)][endCoords.second][1]][fullBoard[endCoords.first+sideToMove*-2+1][endCoords.second][0]][i]==endCoords){
                        auto it = allPieces[fullBoard[endCoords.first+sideToMove*-2+1][endCoords.second][1]][fullBoard[endCoords.first+sideToMove*-2+1][endCoords.second][0]].begin()+i;
                        allPieces[fullBoard[endCoords.first+(sideToMove*-2+1)][endCoords.second][1]][fullBoard[endCoords.first+sideToMove*-2+1][endCoords.second][0]].erase(it);
                        break;
                    }
                }
            }
        }
        fullBoard[allPieces[piece][sideToMove][index].first][allPieces[piece][sideToMove][index].second][0] = -1;
        fullBoard[allPieces[piece][sideToMove][index].first][allPieces[piece][sideToMove][index].second][1] = -1;
        fullBoard[endCoords.first][endCoords.second][0] = sideToMove;
        fullBoard[endCoords.first][endCoords.second][1] = piece;
        allPieces[piece][sideToMove][index].first = endCoords.first;
        allPieces[piece][sideToMove][index].second = endCoords.second;
        return capturedPiece;
    }
    void makeMove(move moveToMake){
        for(int i=0; i<allPieces[moveToMake.movedPieceType][sideToMove].size(); i++){
            if(allPieces[moveToMake.movedPieceType][sideToMove][i].first==moveToMake.startCoords.first &&
            allPieces[moveToMake.movedPieceType][sideToMove][i].second==moveToMake.startCoords.second){
                moveToMake.movedPieceIndex = i;
            }
        }
        fiftyMoveRuleCounter++;
        canEnPassant = false;
        if(moveToMake.movedPieceType==0 && moveToMake.endCoords.first-moveToMake.startCoords.first==sideToMove*4-2){
            canEnPassant = true;
            enPassantSquare = moveToMake.endCoords;
            enPassantSquare.first+=sideToMove*-2+1;
        }
        if(moveToMake.movedPieceType == 5){
            castling[sideToMove] = std::make_pair(false, false);
        }
        if(moveToMake.movedPieceType == 3){
            if(moveToMake.startCoords == std::make_pair(sideToMove*-7+7, 0)){
                castling[sideToMove].first = false;
            }
            if(moveToMake.startCoords == std::make_pair(sideToMove*-7+7, 7)){
                castling[sideToMove].second = false;
            }
        }
        if(moveToMake.endCoords == std::make_pair(sideToMove*7, 0)){
            castling[!sideToMove].first = false;
        }
        if(moveToMake.endCoords == std::make_pair(sideToMove*7, 7)){
            castling[!sideToMove].second = false;
        }
        if(fullBoard[moveToMake.endCoords.first][moveToMake.endCoords.second][0]!=-1){
            moveToMake.capturedPiece = fullBoard[moveToMake.endCoords.first][moveToMake.endCoords.second][1];
            //std::cout << "[" << fullBoard[endCoords.first][endCoords.second][0] << "]";
            for(int i=0; i<allPieces[fullBoard[moveToMake.endCoords.first][moveToMake.endCoords.second][1]][fullBoard[moveToMake.endCoords.first][moveToMake.endCoords.second][0]].size(); i++){
                if(allPieces[fullBoard[moveToMake.endCoords.first][moveToMake.endCoords.second][1]][fullBoard[moveToMake.endCoords.first][moveToMake.endCoords.second][0]][i]==moveToMake.endCoords){
                    auto it = allPieces[fullBoard[moveToMake.endCoords.first][moveToMake.endCoords.second][1]][fullBoard[moveToMake.endCoords.first][moveToMake.endCoords.second][0]].begin()+i;
                    allPieces[fullBoard[moveToMake.endCoords.first][moveToMake.endCoords.second][1]][fullBoard[moveToMake.endCoords.first][moveToMake.endCoords.second][0]].erase(it);
                    break;
                }
            }
        }
        if(moveToMake.endCoords.first==2 || moveToMake.endCoords.first==5){
            if(fullBoard[moveToMake.endCoords.first+sideToMove*-2+1][moveToMake.endCoords.second][0]!=-1 && moveToMake.isEnPassant){
                for(int i=0; i<allPieces[fullBoard[moveToMake.endCoords.first+sideToMove*-2+1][moveToMake.endCoords.second][1]][fullBoard[moveToMake.endCoords.first+sideToMove*-2+1][moveToMake.endCoords.second][0]].size(); i++){
                    if(allPieces[fullBoard[moveToMake.endCoords.first+(sideToMove*-2+1)][moveToMake.endCoords.second][1]][fullBoard[moveToMake.endCoords.first+sideToMove*-2+1][moveToMake.endCoords.second][0]][i]==moveToMake.endCoords){
                        auto it = allPieces[fullBoard[moveToMake.endCoords.first+sideToMove*-2+1][moveToMake.endCoords.second][1]][fullBoard[moveToMake.endCoords.first+sideToMove*-2+1][moveToMake.endCoords.second][0]].begin()+i;
                        allPieces[fullBoard[moveToMake.endCoords.first+(sideToMove*-2+1)][moveToMake.endCoords.second][1]][fullBoard[moveToMake.endCoords.first+sideToMove*-2+1][moveToMake.endCoords.second][0]].erase(it);
                        break;
                    }
                }
            }
        }
        fullBoard[moveToMake.startCoords.first][moveToMake.startCoords.second][0] = -1;
        fullBoard[moveToMake.startCoords.first][moveToMake.startCoords.second][1] = -1;
        fullBoard[moveToMake.endCoords.first][moveToMake.endCoords.second][0] = sideToMove;
        fullBoard[moveToMake.endCoords.first][moveToMake.endCoords.second][1] = moveToMake.movedPieceType;
        allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].first = moveToMake.endCoords.first;
        allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].second = moveToMake.endCoords.second;
        sideToMove = !sideToMove;
    }

    void unMakeLastMove(int movingPiece, int index, std::pair<int, int> originalCoords, bool isEnPassant, int capturedPiece){
        if(capturedPiece!=-1){
            if(isEnPassant){
                fullBoard[allPieces[movingPiece][sideToMove][index].first+(sideToMove*-2+1)][allPieces[movingPiece][sideToMove][index].second][0] = !sideToMove;
                fullBoard[allPieces[movingPiece][sideToMove][index].first+(sideToMove*-2+1)][allPieces[movingPiece][sideToMove][index].second][1] = capturedPiece;
                fullBoard[allPieces[movingPiece][sideToMove][index].first][allPieces[movingPiece][sideToMove][index].second][0] = -1;
                fullBoard[allPieces[movingPiece][sideToMove][index].first][allPieces[movingPiece][sideToMove][index].second][1] = -1;
                allPieces[capturedPiece][!sideToMove].push_back(std::make_pair(allPieces[movingPiece][sideToMove][index].first+(sideToMove*-2+1), allPieces[movingPiece][sideToMove][index].second));
            }
            else{
                fullBoard[allPieces[movingPiece][sideToMove][index].first][allPieces[movingPiece][sideToMove][index].second][0] = !sideToMove;
                fullBoard[allPieces[movingPiece][sideToMove][index].first][allPieces[movingPiece][sideToMove][index].second][1] = capturedPiece;
                allPieces[capturedPiece][!sideToMove].push_back(std::make_pair(allPieces[movingPiece][sideToMove][index].first, allPieces[movingPiece][sideToMove][index].second));
            }
        }
        else{
            fullBoard[allPieces[movingPiece][sideToMove][index].first][allPieces[movingPiece][sideToMove][index].second][0] = -1;
            fullBoard[allPieces[movingPiece][sideToMove][index].first][allPieces[movingPiece][sideToMove][index].second][1] = -1;
        }
        fullBoard[originalCoords.first][originalCoords.second][0] = sideToMove;
        fullBoard[originalCoords.first][originalCoords.second][1] = movingPiece;
        allPieces[movingPiece][sideToMove][index].first = originalCoords.first;
        allPieces[movingPiece][sideToMove][index].second = originalCoords.second;
    }

    void unMakeLastMove(move moveToMake){
        fiftyMoveRuleCounter = moveToMake.originalFiftyMoveCount;
        canEnPassant = moveToMake.originalEnPassantSquare.first!=-1;
        castling = moveToMake.originalCastling;
        sideToMove = !sideToMove;
        for(int i=0; i<allPieces[moveToMake.movedPieceType][sideToMove].size(); i++){
            if(allPieces[moveToMake.movedPieceType][sideToMove][i].first==moveToMake.endCoords.first &&
            allPieces[moveToMake.movedPieceType][sideToMove][i].second==moveToMake.endCoords.second){
                moveToMake.movedPieceIndex = i;
            }
        }
        if(moveToMake.capturedPiece!=-1){
            if(moveToMake.isEnPassant){
                fullBoard[allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].first+(sideToMove*-2+1)][allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].second][0] = !sideToMove;
                fullBoard[allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].first+(sideToMove*-2+1)][allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].second][1] = moveToMake.capturedPiece;
                fullBoard[allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].first][allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].second][0] = -1;
                fullBoard[allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].first][allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].second][1] = -1;
                allPieces[moveToMake.capturedPiece][!sideToMove].push_back(std::make_pair(allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].first+(sideToMove*-2+1), allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].second));
            }
            else{
                fullBoard[allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].first][allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].second][0] = !sideToMove;
                fullBoard[allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].first][allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].second][1] = moveToMake.capturedPiece;
                allPieces[moveToMake.capturedPiece][!sideToMove].push_back(std::make_pair(allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].first, allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].second));
            }
        }
        else{
            fullBoard[allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].first][allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].second][0] = -1;
            fullBoard[allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].first][allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].second][1] = -1;
        }
        fullBoard[moveToMake.startCoords.first][moveToMake.startCoords.second][0] = sideToMove;
        fullBoard[moveToMake.startCoords.first][moveToMake.startCoords.second][1] = moveToMake.movedPieceType;
        allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].first = moveToMake.startCoords.first;
        allPieces[moveToMake.movedPieceType][sideToMove][moveToMake.movedPieceIndex].second = moveToMake.startCoords.second;
    }

    std::vector<move> generateMoves(){
        char x; //for move output
        std::vector<move> legalMoves;
        std::pair<int, int> currentPiece;
        int yChange;
        int xChange;
        int capturedPiece;
        move currentMove;
        currentMove.originalFiftyMoveCount = fiftyMoveRuleCounter;
        currentMove.originalEnPassantSquare = enPassantSquare;
        if(!canEnPassant){currentMove.originalEnPassantSquare = std::make_pair(-1, -1);}
        currentMove.originalCastling = castling;
        bool inCheck = isInCheck();
        //std::cout << "hi";
        //king moves, no castling yet
        for(int i=0; i<allPieces[5][sideToMove].size(); i++){
            currentPiece = allPieces[5][sideToMove][i];
            for(int j=0; j<kingMoves.size(); j++){
                for(int k=0; k<kingMoves[j].size(); k++){
                    yChange = kingMoves[j][k][0];
                    xChange = kingMoves[j][k][1];
                    if(0>currentPiece.first+yChange || 7<currentPiece.first+yChange || 0>currentPiece.second+xChange || 7<currentPiece.second+xChange){
                        break;
                    }
                    if(fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0]==sideToMove){
                        break;
                    }
                    if(xChange==2 || xChange==-2){
                        if(((xChange==2 && castling[sideToMove].second) || (xChange==-2 && castling[sideToMove].first && fullBoard[currentPiece.first+yChange][currentPiece.second+xChange-1][0]==-1)) && (fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0]==-1) && !inCheck){
                            capturedPiece = makeMove(5, i, std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange), false);
                        }
                        else{
                            break;
                        }
                    }
                    else{
                        capturedPiece = makeMove(5, i, std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange), false);
                    }
                    if(!isInCheck()){
                        if(outputMoves){
                            x = currentPiece.second+97;
                            std::cout << x << 8-currentPiece.first;
                            x = currentPiece.second+xChange+97;
                            std::cout << x << 8-(currentPiece.first+yChange) << "\n";
                        }
                        //std::cout << "(" << currentPiece.second+xChange << ", " << currentPiece.first+yChange << ")";
                        currentMove.movedPieceType = 5; currentMove.movedPieceIndex = i; currentMove.endCoords = std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange); currentMove.isEnPassant = false; currentMove.capturedPiece = capturedPiece; currentMove.startCoords = std::make_pair(currentPiece.first, currentPiece.second);
                        legalMoves.push_back(currentMove);
                        unMakeLastMove(5, i, std::make_pair(currentPiece.first, currentPiece.second), false, capturedPiece);
                    }
                    else{unMakeLastMove(5, i, std::make_pair(currentPiece.first, currentPiece.second), false, capturedPiece); break;} //dont castle if there is check on the way
                    if(fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0]==!sideToMove){
                        break;
                    }
                }
            }
        }
        //queen moves
        //std::cout << "|";
        for(int i=0; i<allPieces[4][sideToMove].size(); i++){
            currentPiece = allPieces[4][sideToMove][i];
            for(int j=0; j<queenMoves.size(); j++){
                for(int k=0; k<queenMoves[j].size(); k++){
                    yChange = queenMoves[j][k][0];
                    xChange = queenMoves[j][k][1];
                    if(0>currentPiece.first+yChange || 7<currentPiece.first+yChange || 0>currentPiece.second+xChange || 7<currentPiece.second+xChange){
                        break;
                    }
                    if(fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0]==sideToMove){
                        break;
                    }
                    capturedPiece = makeMove(4, i, std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange), false);
                    if(!isInCheck()){
                        if(outputMoves){
                            x = currentPiece.second+97;
                            std::cout << x << 8-currentPiece.first;
                            x = currentPiece.second+xChange+97;
                            std::cout << x << 8-(currentPiece.first+yChange) << "\n";
                        }
                        //std::cout << "(" << currentPiece.second+xChange << ", " << currentPiece.first+yChange << ")";
                        currentMove.movedPieceType = 4; currentMove.movedPieceIndex = i; currentMove.endCoords = std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange); currentMove.isEnPassant = false; currentMove.capturedPiece = capturedPiece; currentMove.startCoords = std::make_pair(currentPiece.first, currentPiece.second);
                        legalMoves.push_back(currentMove);
                    }
                    unMakeLastMove(4, i, std::make_pair(currentPiece.first, currentPiece.second), false, capturedPiece);
                    if(fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0]==!sideToMove){
                        break;
                    }
                }
            }
        }
        //rook moves
        //std::cout << "|";
        for(int i=0; i<allPieces[3][sideToMove].size(); i++){
            currentPiece = allPieces[3][sideToMove][i];
            for(int j=0; j<rookMoves.size(); j++){
                for(int k=0; k<rookMoves[j].size(); k++){
                    yChange = rookMoves[j][k][0];
                    xChange = rookMoves[j][k][1];
                    if(0>currentPiece.first+yChange || 7<currentPiece.first+yChange || 0>currentPiece.second+xChange || 7<currentPiece.second+xChange){
                        break;
                    }
                    if(fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0]==sideToMove){
                        break;
                    }
                    capturedPiece = makeMove(3, i, std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange), false);
                    if(!isInCheck()){
                        if(outputMoves){
                            x = currentPiece.second+97;
                            std::cout << x << 8-currentPiece.first;
                            x = currentPiece.second+xChange+97;
                            std::cout << x << 8-(currentPiece.first+yChange) << "\n";
                        }
                        //std::cout << "(" << currentPiece.second+xChange << ", " << currentPiece.first+yChange << ")";
                        currentMove.movedPieceType = 3; currentMove.movedPieceIndex = i; currentMove.endCoords = std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange); currentMove.isEnPassant = false; currentMove.capturedPiece = capturedPiece; currentMove.startCoords = std::make_pair(currentPiece.first, currentPiece.second);
                        legalMoves.push_back(currentMove);
                    }
                    unMakeLastMove(3, i, std::make_pair(currentPiece.first, currentPiece.second), false, capturedPiece);
                    if(fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0]==!sideToMove){
                        break;
                    }
                }
            }
        }
        //bishop moves
        //std::cout << "|";
        for(int i=0; i<allPieces[2][sideToMove].size(); i++){
            currentPiece = allPieces[2][sideToMove][i];
            for(int j=0; j<bishopMoves.size(); j++){
                for(int k=0; k<bishopMoves[j].size(); k++){
                    yChange = bishopMoves[j][k][0];
                    xChange = bishopMoves[j][k][1];
                    if(0>currentPiece.first+yChange || 7<currentPiece.first+yChange || 0>currentPiece.second+xChange || 7<currentPiece.second+xChange){
                        break;
                    }
                    if(fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0]==sideToMove){
                        break;
                    }
                    capturedPiece = makeMove(2, i, std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange), false);
                    if(!isInCheck()){
                        if(outputMoves){
                            x = currentPiece.second+97;
                            std::cout << x << 8-currentPiece.first;
                            x = currentPiece.second+xChange+97;
                            std::cout << x << 8-(currentPiece.first+yChange) << "\n";
                        }
                        //std::cout << "(" << currentPiece.second+xChange << ", " << currentPiece.first+yChange << ")";
                        currentMove.movedPieceType = 2; currentMove.movedPieceIndex = i; currentMove.endCoords = std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange); currentMove.isEnPassant = false; currentMove.capturedPiece = capturedPiece; currentMove.startCoords = std::make_pair(currentPiece.first, currentPiece.second);
                        legalMoves.push_back(currentMove);
                    }
                    unMakeLastMove(2, i, std::make_pair(currentPiece.first, currentPiece.second), false, capturedPiece);
                    if(fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0]==!sideToMove){
                        break;
                    }
                }
            }
        }
        //knight moves
        //std::cout << "|";
        for(int i=0; i<allPieces[1][sideToMove].size(); i++){
            currentPiece = allPieces[1][sideToMove][i];
            for(int j=0; j<knightMoves.size(); j++){
                for(int k=0; k<knightMoves[j].size(); k++){
                    yChange = knightMoves[j][k][0];
                    xChange = knightMoves[j][k][1];
                    if(0>currentPiece.first+yChange || 7<currentPiece.first+yChange || 0>currentPiece.second+xChange || 7<currentPiece.second+xChange){
                        break;
                    }
                    //std::cout << "\nDEBUG VALUES1:" << fullBoard[5][0][0] << "\n" << sideToMove;
                    if(fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0]==sideToMove){
                        break;
                    }
                    //std::cout << "\nDEBUG VALUES2:" << fullBoard[5][0][0];
                    capturedPiece = makeMove(1, i, std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange), false);
                    if(!isInCheck()){
                        if(outputMoves){
                            x = currentPiece.second+97;
                            std::cout << x << 8-currentPiece.first;
                            x = currentPiece.second+xChange+97;
                            std::cout << x << 8-(currentPiece.first+yChange) << "\n";
                        }
                        //std::cout << "(" << currentPiece.second+xChange << ", " << currentPiece.first+yChange << ")";
                        currentMove.movedPieceType = 1; currentMove.movedPieceIndex = i; currentMove.endCoords = std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange); currentMove.isEnPassant = false; currentMove.capturedPiece = capturedPiece; currentMove.startCoords = std::make_pair(currentPiece.first, currentPiece.second);
                        legalMoves.push_back(currentMove);
                    }
                    unMakeLastMove(1, i, std::make_pair(currentPiece.first, currentPiece.second), false, capturedPiece);
                    //std::cout << "\nDEBUG VALUES3:" << fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0] << " " << !sideToMove;
                    //std::cout << (fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0]==!sideToMove);
                    if(fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0]==!sideToMove){
                        //std::cout << "\nDEBUG MESSAGE1";
                        break;
                    }
                }
            }
        }
        //pawn moves
        //std::cout << "|";
        //std::cout << allPieces[0][sideToMove].size();
        bool isEnPassant;
        for(int i=0; i<allPieces[0][sideToMove].size(); i++){
            currentPiece = allPieces[0][sideToMove][i];
            for(int j=0; j<pawnMoves[sideToMove].size(); j++){
                for(int k=0; k<pawnMoves[sideToMove][j].size(); k++){
                    yChange = pawnMoves[sideToMove][j][k][0];
                    xChange = pawnMoves[sideToMove][j][k][1];
                    isEnPassant = false;
                    if(0>currentPiece.first+yChange || 7<currentPiece.first+yChange || 0>currentPiece.second+xChange || 7<currentPiece.second+xChange){
                        break;
                    }
                    if(fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0]!=-1 && xChange==0){
                        break;
                    }
                    if(fullBoard[currentPiece.first+yChange][currentPiece.second+xChange][0]!=!sideToMove && xChange!=0 && (!canEnPassant || !(canEnPassant && std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange)==enPassantSquare))){
                        break;
                    }
                    if(canEnPassant && std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange)==enPassantSquare && xChange!=0){
                        isEnPassant = true;
                        capturedPiece = makeMove(0, i, std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange), true);
                    }
                    else{
                        capturedPiece = makeMove(0, i, std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange), false);
                    }
                    //std::cout << currentPiece.second << "\n";
                    if(!isInCheck()){
                        //std::cout << "hi";
                        if(outputMoves){
                            x = currentPiece.second+97;
                            std::cout << x << 8-currentPiece.first;
                            x = currentPiece.second+xChange+97;
                            std::cout << x << 8-(currentPiece.first+yChange) << "\n";
                        }
                        //std::cout << 
                        //std::cout << "(" << currentPiece.second+xChange << ", " << currentPiece.first+yChange << ")";
                        currentMove.movedPieceType = 0; currentMove.movedPieceIndex = i; currentMove.endCoords = std::make_pair(currentPiece.first+yChange, currentPiece.second+xChange); currentMove.isEnPassant = isEnPassant; currentMove.capturedPiece = capturedPiece; currentMove.startCoords = std::make_pair(currentPiece.first, currentPiece.second);
                        legalMoves.push_back(currentMove);
                    }
                    unMakeLastMove(0, i, std::make_pair(currentPiece.first, currentPiece.second), isEnPassant, capturedPiece);
                    if(currentPiece.first!=3.5+2.5*(sideToMove*-2+1)){
                        break;
                    }
                }
            }
        }
        return legalMoves;
    }
};
}