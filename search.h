#pragma once
#include "evaluation.h"
#include <fstream>
#include <time.h>
#include <math.h>
#include <memory>
#include <chrono>
#include <deque>

#if DATAGEN >= 1
  std::string dataFolderPath = "C:/Users/kjlji/OneDrive/Documents/VSCode/C++/AuroraChessEngine-main/data";
#endif

namespace search{

//parameters for search
enum backpropagationStrategy{AVERAGE, MINIMAX};
backpropagationStrategy backpropStrat = MINIMAX;

//float eP[12] = {1.6301532566281178, 0.3415019889631426, 1.462459315326555, 0.09830134955092976, 0.3670339438501686, 0.5028838849947221, 0.28917477475978387, 1.581231015213, 0.2747746463404976, 0.9214915071600298, 0.14796697203232123, 1.2260899419271722}; //exploration parameters

#if DATAGEN == 0
uint8_t seldepth = 0;
uint32_t depth = 0;
uint32_t startNodes = 0;
#endif

void init(){
  Aurora::initOptions();
  evaluation::init();
  zobrist::init();
  srand(time(NULL));
  std::cout.precision(10);
}

struct Node;

#pragma pack(push, 1)
struct Edge{
  Node* child;
  float value;
  chess::Move edge;

  Edge() : child(nullptr), value(-2), edge(chess::Move()) {}
  Edge(chess::Move move) : child(nullptr), value(-2), edge(move) {}
};
#pragma pack(pop)

struct Node{
  std::vector<Edge> children;
  Node* parent;

  //For LRU tree management
  Node* backLink = nullptr; //back = node use less recently
  Node* forwardLink = nullptr; //forward = node used more recently
  //For Tree Reuse
  Node* newAddress = nullptr;

  uint32_t visits;
  int iters;
  float avgValue;

  bool isTerminal;
  uint8_t index;
  //For Tree Reuse
  bool mark = false;

  Node(Node* parent) :
  parent(parent),
  visits(0), iters(0), avgValue(-2), isTerminal(false) {}

  Node() : parent(nullptr), visits(0), iters(0), avgValue(-2), isTerminal(false) {}
};

Edge findBestEdge(Node* parent){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
  Edge currBestMove = parent->children[0];

  for(int i=0; i<parent->children.size(); i++){
    if(parent->children[i].value < currBestValue){
      currBestValue = parent->children[i].value;
      currBestMove = parent->children[i];
    }
  }

  return currBestMove;
}

Node* findBestChild(Node* parent){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
  Node* currBestMove = parent->children[0].child;

  for(int i=0; i<parent->children.size(); i++){
    if(parent->children[i].value < currBestValue){
      currBestValue = parent->children[i].value;
      currBestMove = parent->children[i].child;
    }
  }

  return currBestMove;
}

float findBestValue(Node* parent){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective

  for(int i=0; i<parent->children.size(); i++){
    currBestValue = std::min(currBestValue, parent->children[i].value);
  }

  return currBestValue;
}

struct TTEntry{
  float val;
  uint32_t hash;

  TTEntry() : val(-2), hash(0) {}
};

struct Tree{
  std::deque<Node> tree;
  std::vector<TTEntry> TT;
  Node* root = nullptr;
  uint64_t sizeLimit = 0;
  uint64_t currSize = 0;
  Node* tail = nullptr;
  Node* head = nullptr;

  TTEntry* getTTEntry(U64 hash){
    return &TT[hash % TT.size()];
  }

  void setHash(){
    uint32_t hashMb = Aurora::options["Hash"].value;
    sizeLimit = 1000000 * hashMb * (Aurora::options["TTHash"].value ? 1 : 0.8);
    TT.clear();
    uint32_t ttHashBytes = Aurora::options["TTHash"].value
                              ? Aurora::options["TTHash"].value * 1000000
                              : hashMb * 1000000 * 0.2;
    TT.resize(std::max(uint32_t(1), uint32_t(ttHashBytes / sizeof(TTEntry))));
  }

  float getHashfull(){
    float treeHashfull = sizeLimit > 0 ? float(currSize) / sizeLimit : 0;

    float ttHashfull = 0;
    int numTTEntriesToCheck = std::min(1000, int(TT.size()));
    for(int i=0; i<numTTEntriesToCheck; i++){
      if(TT[i].val != -2){
        ttHashfull += 1;
      }
    }
    ttHashfull /= numTTEntriesToCheck;

    float totalHash = TT.size() * sizeof(TTEntry) + sizeLimit;
    return treeHashfull * (sizeLimit / totalHash) + ttHashfull * (TT.size()*sizeof(TTEntry) / totalHash);
  }

  //for debug purposes
  void passthrough(){
    Node* currNode = tail;
    while(currNode){
      currNode = currNode->forwardLink;
    }
    currNode = head;
    while(currNode){
      currNode = currNode->backLink;
    }
    return;
  }

  void moveToHead(Node* node){
    if(head == node){
      return;
    }
    if(tail == node && node->forwardLink){
      tail = node->forwardLink;
    }
    if(node->backLink){
      node->backLink->forwardLink = node->forwardLink;
    }
    if(node->forwardLink){
      node->forwardLink->backLink = node->backLink;
    }
    head->forwardLink = node;
    node->backLink = head;
    node->forwardLink = nullptr;
    head = node;
  }

  Node* push_back(Node node){
    if(sizeLimit != 0 && currSize >= sizeLimit){
      assert(tail);
      Node* currTail = tail;
      for(int i=0; i<currTail->children.size(); i++){
        currSize -= sizeof(Edge);
        if(currTail->children[i].child){
          currTail->children[i].child->parent = nullptr;
        }
      }
      if(currTail->parent){
        //Update the 16th bit in the chess::Move to indicate that the child was pruned
        currTail->parent->children[currTail->index].edge.value |= 1 << 15;
        currTail->parent->children[currTail->index].child = nullptr;
      }
      if(currTail->forwardLink){
        currTail->forwardLink->backLink = nullptr;
      }

      tail = currTail->forwardLink;
      tail->backLink = nullptr;

      *currTail = node;
      currTail->children.shrink_to_fit(); //Free Memory
      head->forwardLink = currTail;
      currTail->backLink = head;
      currTail->forwardLink = nullptr;
      head = currTail;
      return head;
    }
    else{
      tree.push_back(node);
      currSize += sizeof(Node);
      tree.back().backLink = head;
      if(head){
        head->forwardLink = &tree.back();
      }
      else{
        tail = &tree.back();
      }
      head = &tree.back();
      return &tree.back();
    }
  }
};

void destroyTree(Tree& tree){
  tree.tree.clear();
  tree.root = nullptr;
  tree.tail = nullptr;
  tree.head = nullptr;
  tree.currSize = 0;
}

uint64_t markSubtree(Node* node, bool isSubtreeRoot = true, bool unmarked = true){
  uint64_t markedNodes = 0;

  if(isSubtreeRoot){
    unmarked = node->mark;
  }
  if(node){
    node->mark = !unmarked;
    markedNodes++;
    for(int i=0; i<node->children.size(); i++){
      markedNodes += markSubtree(node->children[i].child, false, unmarked);
    }
  }
  return markedNodes;
}

//Returns a pointer to the new root, which is different from the pointer given as a parameter because of the garbage collection
Node* moveRootToChild(Tree& tree, Node* newRoot, Node* currRoot){
  //LISP 2 Garbage Collection Algorithm (https://en.wikipedia.org/wiki/Mark%E2%80%93compact_algorithm#LISP_2_algorithm)
  //Mark all nodes which we want to keep
  uint64_t markedNodes = markSubtree(newRoot);
  bool marked = newRoot->mark;

  //Index of the next unreserved address in tree.tree
  uint64_t freePointer = 0;

  //Reserve addresses for all nodes we want to keep
  for(uint32_t i=0; i<tree.tree.size(); i++){
    Node* livePointer = &tree.tree[i];
    if(livePointer->mark == marked){
      livePointer->newAddress = &tree.tree[freePointer];
      freePointer++;
    }
  }

  Node* newRootNewAddress = newRoot->newAddress;

  //Update LRU links to not include nodes we're about to discard
  for(Node& node : tree.tree){
    if(node.mark == marked){
      Node* currNode = node.backLink;
      while(currNode && currNode->mark == !marked){
        currNode = currNode->backLink;
      }
      if(!currNode){
        node.backLink = nullptr;
        tree.tail = &node;
      }
      else{
        node.backLink = currNode;
        currNode->forwardLink = &node;
      }

      currNode = node.forwardLink;
      while(currNode && currNode->mark == !marked){
        currNode = currNode->forwardLink;
      }
      if(!currNode){
        node.forwardLink = nullptr;
        tree.head = &node;
      }
      else{
        node.forwardLink = currNode;
        currNode->backLink = &node;
      }
    }
  }

  tree.currSize = 0;

  //Update pointers to new addresses
  for(Node& node : tree.tree){
    if(node.mark == marked){
      tree.currSize += sizeof(Node);
      if(node.parent){
        assert(&node == newRoot || node.parent->mark == marked);
        node.parent = node.parent->newAddress;
      }
      if(node.backLink){
        node.backLink = node.backLink->newAddress;
      }
      if(node.forwardLink){
        node.forwardLink = node.forwardLink->newAddress;
      }
      for(int i=0; i<node.children.size(); i++){
        tree.currSize += sizeof(Edge);
        if(node.children[i].child){
          assert(node.children[i].child->mark == marked);
          node.children[i].child = node.children[i].child->newAddress;
        }
      }
    }
  }

  tree.head = tree.head->newAddress;
  tree.tail = tree.tail->newAddress;

  //Move nodes to new addresses
  for(uint32_t i=0; i<tree.tree.size(); i++){
    Node* livePointer = &tree.tree[i];
    if(livePointer->mark == marked){
      *(livePointer->newAddress) = *livePointer;
    }
    livePointer++;
  }

  tree.tree.resize(markedNodes);

  return newRootNewAddress;
}

uint8_t selectEdge(Node* parent, bool isRoot, float rootExpl, float expl){
  float maxPriority = -2;
  uint8_t maxPriorityNodeIndex = 0;

  const float parentVisitsTerm = (isRoot ? rootExpl : expl)*std::log(parent->visits)*std::sqrt(std::log(parent->visits));

  for(int i=0; i<parent->children.size(); i++){
    Edge currEdge = parent->children[i];
    Node* currNode = currEdge.child;

    //Avoid exploring terminal nodes
    if(currNode && currNode->isTerminal){
      continue;
    }

    //We can make a guess about how many visits a node had before it was pruned by LRU
    bool isLRUPruned = parent->children[i].edge.value & (1 << 15);

    float currPriority = -(currNode ? currNode->avgValue : currEdge.value)+
      (parent->visits*0.0004 > (currNode ? currNode->visits : 1) ? 2 : 1)*
      parentVisitsTerm/std::sqrt(currNode ? currNode->visits : (isLRUPruned ? 14 : 1));

    assert(currPriority>=-1);

    if(currPriority>maxPriority){
      maxPriority = currPriority;
      maxPriorityNodeIndex = i;
    }
  }

  return maxPriorityNodeIndex;
}

void expand(Tree& tree, Node* parent, chess::MoveList& moves){
  if(moves.size()==0){return;}

  parent->children.resize(moves.size());
  tree.currSize += moves.size() * sizeof(Edge);

  for(uint16_t i=0; i<moves.size(); i++){
    parent->children[i] = Edge(moves[i]);
  }
}

template<int numHiddenNeurons>
float playout(Tree& tree,chess::Board& board, evaluation::NNUE<numHiddenNeurons>& nnue){
  //First, check if position is terminal
  chess::gameStatus _gameStatus = chess::getGameStatus(board, chess::isLegalMoves(board));
  assert(-1<=_gameStatus && 2>=_gameStatus);
  if(_gameStatus != chess::ONGOING){
    return _gameStatus;
  }

  //Next, check TBs
  chess::gameStatus tbResult = chess::probeWdlTb(board);
  if(tbResult != chess::ONGOING){
    return tbResult;
  }

  //Next, check TT
  TTEntry* entry = tree.getTTEntry(board.history[board.halfmoveClock]);
  if(entry->hash == (board.history[board.halfmoveClock] >> 32) && entry->val != -2){
    return entry->val;
  }

  //Next, do qSearch
  float eval = evaluation::cpToVal(evaluation::evaluate(board, nnue));
  entry->hash = (board.history[board.halfmoveClock] >> 32);
  entry->val = eval;

  assert(-1<=eval && 1>=eval);
  return eval;
}

void backpropagate(Tree& tree, float result, std::vector<std::pair<Edge*, U64>>& edges, uint8_t visits, bool forceResult, bool runFindBestMove, bool continueBackprop, float valChangedMinWeight, float valSameMinWeight){
  //Backpropagate results
  if(edges.size() == 0){return;}

  std::pair<Edge*, U64> p = edges.back();
  Edge* currEdge = p.first; U64 hash = p.second;
  edges.pop_back();

  currEdge->child->visits += visits;

  float oldCurrNodeValue = 2;

  if(backpropStrat == AVERAGE){
    assert(0);
    // currEdge->value = (currEdge->value*currEdge->visits+result)/(currEdge->visits+1);
    // result = -result;
  }
  else if(backpropStrat == MINIMAX){
    //We only need to backpropagate two types of results here: the current best child becomes worse, or there is a new best child
    if(continueBackprop){
      //If currEdge is the best move and is backpropagated to become worse, we need to run findBestValue for the parent of currEdge
      oldCurrNodeValue = 2;
      if(currEdge->child->parent && edges.size() > 0 && -currEdge->value == edges.back().first->value){oldCurrNodeValue = currEdge->value;}

      //If the result is worse than the current value, there is no point in continuing the backpropagation, other than to add visits to the nodes
      if(result <= currEdge->value && !runFindBestMove && !forceResult){
        continueBackprop = false;

        currEdge->child->iters++;
        float newValWeight = fminf(1.0, fmaxf(valSameMinWeight, 1.0/currEdge->child->iters));
        currEdge->child->avgValue = currEdge->child->avgValue*(1-newValWeight) + currEdge->value*newValWeight;

        TTEntry* entry = tree.getTTEntry(hash);
        entry->hash = hash >> 32;
        entry->val = currEdge->value;

        backpropagate(tree, result, edges, visits, false, runFindBestMove, continueBackprop, valChangedMinWeight, valSameMinWeight);
        return;
      }

      currEdge->value = runFindBestMove ? -findBestValue(currEdge->child) : result;

      assert(-1<=currEdge->value && 1>=currEdge->value);

      runFindBestMove = currEdge->value > oldCurrNodeValue; //currEdge(which used to be the best child)'s value got worse from currEdge's parent's perspective

      result = -currEdge->value;

      currEdge->child->iters++;
      float newValWeight = fminf(1.0, fmaxf(valChangedMinWeight, 1.0/currEdge->child->iters));
      currEdge->child->avgValue = currEdge->child->avgValue*(1-newValWeight) + currEdge->value*newValWeight;
    }
    else{
      currEdge->child->iters++;
      float newValWeight = fminf(1.0, fmaxf(valSameMinWeight, 1.0/currEdge->child->iters));
      currEdge->child->avgValue = currEdge->child->avgValue*(1-newValWeight) + currEdge->value*newValWeight;
    }
  }

  TTEntry* entry = tree.getTTEntry(hash);
  entry->hash = hash >> 32;
  entry->val = currEdge->value;

  backpropagate(tree, result, edges, visits, false, runFindBestMove, continueBackprop, valChangedMinWeight, valSameMinWeight);
}

int previousVisits = 0;
int previousElapsed = 0;

void printSearchInfo(Tree& tree, std::chrono::steady_clock::time_point start, bool finalResult){
  Node* root = tree.root;
  if(Aurora::options["outputLevel"].value==3){
    std::cout << "NODES: " << root->visits;
    #if DATAGEN == 0
    std::cout << " SELDEPTH: " << int(seldepth) <<"\n";
    #endif

    std::cout.precision(5);
    for(int i=0; i<root->children.size(); i++){
      Edge currEdge = root->children[i];
      std::cout << "\033[1;4m" << currEdge.edge.toStringRep() <<
                  "\033[0m: \033[1;4mQ\033[0m:" << -currEdge.value <<
                  " \033[1;4mA\033[0m:" << -(currEdge.child ? currEdge.child->avgValue : -2) <<
                  " \033[1;4mI\033[0m:" << (currEdge.child ? currEdge.child->iters : 0) <<
                  " \033[1;4mN\033[0m:" << (currEdge.child ? currEdge.child->visits : 1) <<
                  " \033[1;4mPV\033[0m:";
      Node* pvNode = root->children[i].child;
      while(pvNode && pvNode->children.size() > 0){
        Edge pvEdge = findBestEdge(pvNode);
        std::cout << pvEdge.edge.toStringRep() << " ";
        pvNode = pvEdge.child;
      }
      std::cout << std::endl;
    }
    std::cout.precision(10);
  }

  if(Aurora::options["outputLevel"].value >= 2 || (finalResult && Aurora::options["outputLevel"].value >= 1)){
    std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;

    std::cout << "info ";
    #if DATAGEN == 0
    std::cout << "depth " << (root->visits == startNodes ? 0 : int(depth / (root->visits - startNodes))) <<
                " seldepth " << int(seldepth) << " ";
    #endif
    std::cout << "nodes " << root->visits <<
    " score cp " << evaluation::valToCp(-findBestValue(root)) <<
    " hashfull " << int(tree.getHashfull()*1000) <<
    " nps " << std::round((root->visits-previousVisits)/(elapsed.count()-previousElapsed)) <<
    " time " << std::round(elapsed.count()*1000) <<
    " pv ";
    Node* pvNode = root;
    while(pvNode && pvNode->children.size() > 0){
      Edge pvEdge = findBestEdge(pvNode);
      std::cout << pvEdge.edge.toStringRep() << " ";
      pvNode = pvEdge.child;
    }
    std::cout << std::endl;

    previousVisits = root->visits; previousElapsed = elapsed.count();
  }
}

//Code relating to the time manager
enum timeManagementType{
  FOREVER,
  TIME,
  NODES
};

struct timeManagement{
  timeManagementType tmType = FOREVER;
  float hardLimit;
  float limit; //For FOREVER, this does not matter. For Nodes, this is the amount of nodes. For Time, it is the amount of seconds
  timeManagement(timeManagementType _tmType, uint32_t _limit = 0): tmType(_tmType), limit(_limit) {}
  timeManagement(): tmType(FOREVER), limit(0){}
};

//The main search function
void search(chess::Board& rootBoard, timeManagement tm, Tree& tree){
  auto start = std::chrono::steady_clock::now();

  tree.setHash();
  #if DATAGEN != 1
  std::cout << "info string starting search with max tree size " <<
            (tree.sizeLimit == 0 ? "unlimited" : std::to_string(tree.sizeLimit/1000000.0)) << " mb "
            << "and TT size " <<
            (tree.TT.size()*sizeof(TTEntry)/1000000.0) << " mb"
            << std::endl;
  #endif

  if(!tree.root){tree.push_back(Node()); tree.root = &tree.tree[tree.tree.size()-1];}

  #if DATAGEN == 0
  seldepth = 0;
  depth = 0;
  startNodes = tree.root->visits;
  #endif

  evaluation::NNUE<NNUEhiddenNeurons> nnue(evaluation::_NNUEparameters);

  Node* currNode = tree.root;
  
  //For Printing Search Info
  int lastNodeCheck = 1;
  std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;
  previousVisits = tree.root->visits;
  previousElapsed = 0;

  if(chess::getGameStatus(rootBoard, chess::isLegalMoves(rootBoard)) != chess::ONGOING){
    #if DATAGEN != 1
      std::cout << "bestmove a1a1" << std::endl;
    #endif
    return;
  }

  //For Time Management
  int bestMoveChanges = 0;
  float bestMoveChangesMultiplier = 1;
  chess::Move currBestMove;

  //First, Check TBs
  chess::Move tbMove = chess::probeDtzTb(rootBoard);
  if(tbMove.value){
    chess::gameStatus result = chess::probeWdlTb(rootBoard);
    chess::MoveList moves(rootBoard);
    expand(tree, tree.root, moves);
    for(int i=0; i<moves.size(); i++){
      if(moves[i] == tbMove){
        tree.root->children[i].value = -result+0.001;
      }
      else{
        tree.root->children[i].value = 1;
      }
    }
    tree.root->visits = 1;
    tm.tmType = NODES;
    tm.limit = -1;
  }

  //Get options
  float rootExpl = Aurora::options["rootExplorationFactor"].value;
  float expl = Aurora::options["explorationFactor"].value;
  float valChangedMinWeight = Aurora::options["valChangedMinWeight"].value;
  float valSameMinWeight = Aurora::options["valSameMinWeight"].value;

  while((tm.tmType == FOREVER) || (elapsed.count()<fminf(tm.limit*bestMoveChangesMultiplier, tm.hardLimit) && tm.tmType == TIME) || (tree.root->visits<tm.limit && tm.tmType == NODES)){
    chess::Board board = rootBoard;

    int currDepth = 0;
    currNode = tree.root; tree.moveToHead(tree.root);
    Edge* currEdge;
    std::vector<std::pair<Edge*, U64>> traversePath;

    //Traverse the search tree
    while(currNode->children.size() > 0){
      currDepth++;
      
      //Move all children nodes to the front of LRU
      for(int i=0; i<currNode->children.size(); i++){
        if(currNode->children[i].child != nullptr){
          tree.moveToHead(currNode->children[i].child);
        }
      }

      //Select Child Node to explore
      uint8_t currEdgeIndex = selectEdge(currNode, currNode == tree.root, rootExpl, expl);

      currEdge = &currNode->children[currEdgeIndex];
      chess::makeMove(board, currEdge->edge);
      traversePath.push_back({currEdge, board.history[board.halfmoveClock]});

      //If we only had a child edge before, create the corresponding child node
      if(currEdge->child == nullptr){
        currEdge->child = tree.push_back(Node(currNode));
        currEdge->child->index = currEdgeIndex;
        currEdge->child->mark = currNode->mark;
        currEdge->child->visits = 1;
        currEdge->child->iters = 1;
        currEdge->child->avgValue = currEdge->value;
      }

      currNode = currEdge->child;
    }

    //Expand & Backpropagate new values
    if(currNode->isTerminal){
      #if DATAGEN == 0
      depth += currDepth;
      #endif
      tree.root->visits += 1;
      backpropagate(tree, currEdge->value, traversePath, 1, true, false, true, valChangedMinWeight, valSameMinWeight);
    }
    else{
      //Reached a leaf node
      currDepth++;

      //Make sure game isn't terminal
      chess::MoveList moves(board);
      if(chess::getGameStatus(board, moves.size()!=0) != chess::ONGOING){
        assert(currEdge->value>=-1);
        currNode->isTerminal=true;
        continue;
      }

      //Create new child edges
      expand(tree, currNode, moves);

      //Get values for all created edges
      Node* parentNode = currNode; //This will be where the backpropagation starts

      float currBestValue = 2;

      nnue.refreshAccumulator(board);
      std::array<std::array<int16_t, NNUEhiddenNeurons>, 2> currAccumulator = nnue.accumulator;

      for(int i=0; i<parentNode->children.size(); i++){
        currEdge = &parentNode->children[i];

        chess::Board movedBoard = board;

        nnue.accumulator = currAccumulator;
        nnue.updateAccumulator(movedBoard, currEdge->edge);

        float result = playout(tree, movedBoard, nnue);
        assert(-1<=result && 1>=result);
        currEdge->value = std::max(std::min(result, 1.0f), -1.0f);
        currBestValue = fminf(currBestValue, currEdge->value);
      }

      int visits = 0;
      for(int i=0; i<parentNode->children.size(); i++){
        if(parentNode->children[i].value <= currBestValue + 0.04){visits++;}
      }
      assert(visits >= 1);
      
      #if DATAGEN == 0
      depth += currDepth*visits;
      #endif

      //Update root stats, since backpropagation doesn't reach the root
      tree.root->visits += visits;
      tree.root->iters += 1;

      //Backpropagate best value
      backpropagate(tree, -currBestValue, traversePath, visits, true, false, true, valChangedMinWeight, valSameMinWeight);
    }

    #if DATAGEN == 0
    if(currDepth > seldepth){seldepth = currDepth;}
    #endif

    //Output some information on the search occasionally
    elapsed = std::chrono::steady_clock::now() - start;
    #if DATAGEN != 1
      if(elapsed.count() >= lastNodeCheck*2){
        lastNodeCheck++;
        printSearchInfo(tree, start, false);
      }
    #endif

    //Decide if we want to search longer or shorter depending on how much the best move has changed
    if(findBestEdge(tree.root).edge.value != currBestMove.value){
      bestMoveChanges++;
      currBestMove = findBestEdge(tree.root).edge;
    }

    double expectedBestMoveChanges = 0.26061644 * (std::pow(tree.root->visits, 0.54) - std::pow(startNodes, 0.54));
    bestMoveChangesMultiplier = fmaxf(fminf(bestMoveChanges / expectedBestMoveChanges, 2), 0.2);
  }

  //Output the final result of the search
  #if DATAGEN != 1
    printSearchInfo(tree, start, true);
    std::cout << "\nbestmove " << findBestEdge(tree.root).edge.toStringRep() << std::endl;
  #endif

  return;
}

//Same as chess::makeMove except we move the root so we can keep nodes from an earlier search
//Parameter "board" must be different than parameter "rootBoard"
void makeMove(chess::Board& board, chess::Move move, chess::Board& rootBoard, Tree& tree){
  if(tree.root == nullptr || zobrist::getHash(board) != zobrist::getHash(rootBoard)){chess::makeMove(board, move); return;}

  chess::makeMove(board, move);

  Edge newRootEdge = Edge(chess::Move());
  for(int i=0; i<tree.root->children.size(); i++){
    newRootEdge = tree.root->children[i];
    if(newRootEdge.edge == move){break;}
  }
  Node* newRoot = newRootEdge.child;

  if(newRoot == nullptr){tree.root = nullptr; destroyTree(tree); return;}

  tree.root = moveRootToChild(tree, newRoot, tree.root);

  tree.root->parent = nullptr; tree.root->visits--;//Visits needs to be subtracted by 1 to remove the visit which added the node

  chess::makeMove(rootBoard, move);
}

}//namespace search