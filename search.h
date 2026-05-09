#pragma once
#include "evaluation.h"
#include <algorithm>
#include <fstream>
#include <time.h>
#include <math.h>
#include <memory>
#include <chrono>
#include <deque>
#include <iomanip>

#if DATAGEN >= 1
  std::string dataFolderPath = "C:/Users/kjlji/OneDrive/Documents/VSCode/C++/AuroraChessEngine-main/data";
#endif

namespace search{

inline void init(){
  evaluation::init();
  zobrist::init();
  std::cout.precision(10);
}

struct Node;

#pragma pack(push, 1)
struct Edge{
  uint32_t childIdx;
  float value;
  chess::Move edge;

  Edge() : childIdx(UINT32_MAX), value(-2) {}
  Edge(chess::Move move) : childIdx(UINT32_MAX), value(-2), edge(move) {}
};
#pragma pack(pop)

struct Node{
  std::vector<Edge> children;
  uint32_t parentIdx = UINT32_MAX;

  //For LRU tree management
  uint32_t backIdx = UINT32_MAX; //back = node use less recently
  uint32_t forwardIdx = UINT32_MAX; //forward = node used more recently

  uint32_t visits;
  int iters;
  float avgValue;
  float sumSquaredVals = 0;

  bool isTerminal;
  uint8_t index = 0;
  //For Tree Reuse
  bool mark = false;

  Node(uint32_t parentIdx) :
    parentIdx(parentIdx),
    visits(0), iters(0), avgValue(-2), isTerminal(false) {}

  Node() :
    visits(0), iters(0), avgValue(-2), isTerminal(false) {}

  float variance() const{
    return (sumSquaredVals - (avgValue * avgValue));
  }
};

struct TTEntry{
  float val = -2;
  uint32_t hash = 0;
};

struct Tree{
  std::vector<Node> tree;
  std::vector<TTEntry> TT;
  uint32_t rootIdx = UINT32_MAX;
  uint64_t sizeLimit = 0;
  uint64_t currSize = 0;
  uint32_t tailIdx = UINT32_MAX;
  uint32_t headIdx = UINT32_MAX;

  // Helper accessors
  Node* getNode(uint32_t idx) {
    return idx == UINT32_MAX ? nullptr : &tree[idx];
  }
  uint32_t getIdx(Node* node) {
    return node == nullptr ? UINT32_MAX : uint32_t(node - tree.data());
  }

  Node* root() { return getNode(rootIdx); }
  Node* head() { return getNode(headIdx); }
  Node* tail() { return getNode(tailIdx); }

  //Used for nps calculations in printing search info
  int previousVisits = 0;
  float previousElapsed = 0;

  uint8_t seldepth = 0;
  uint32_t depth = 0;

  //Nodes at the start of a search
  uint32_t startNodes = 0;

  TTEntry* getTTEntry(U64 hash){
    return &TT[hash % TT.size()];
  }

  void setHash(){
    float hashMb = Aurora::hash.value;
    const int BYTES_PER_MB = 1000000;
    sizeLimit = BYTES_PER_MB * hashMb * (Aurora::ttHash.value ? 1 : (1-Aurora::ttHashProportion.value));
    uint32_t ttHashBytes = Aurora::ttHash.value
                              ? Aurora::ttHash.value * BYTES_PER_MB
                              : hashMb * BYTES_PER_MB * Aurora::ttHashProportion.value;
    size_t targetEntries = std::max<size_t>(1, ttHashBytes / sizeof(TTEntry));
    if(TT.size() != targetEntries){
      TT.clear();
      TT.resize(targetEntries);
    }
    tree.reserve(sizeLimit / sizeof(Node));
  }

  float getTreefull(){
    return sizeLimit > 0 ? float(currSize) / sizeLimit : 0;
  }

  float getTTfull(){
    int numTTEntriesToCheck = std::min(1000, int(TT.size()));
    float ttHashfull = 0;
    for(int i=0; i<numTTEntriesToCheck; i++){
      if(TT[i].val != -2){
        ttHashfull += 1;
      }
    }
    ttHashfull /= numTTEntriesToCheck;
    return ttHashfull;
  }

  float getHashfull(){
    float treeHashfull = getTreefull();
    float ttHashfull = getTTfull();
    float totalHash = (TT.size() * sizeof(TTEntry)) + sizeLimit;
    return (treeHashfull * (sizeLimit / totalHash)) + (ttHashfull * ((TT.size() * sizeof(TTEntry)) / totalHash));
  }

  //for debug purposes
  void passthrough() const{
    uint32_t currIdx = tailIdx;
    while(currIdx != UINT32_MAX){
      currIdx = tree[currIdx].forwardIdx;
    }
    currIdx = headIdx;
    while(currIdx != UINT32_MAX){
      currIdx = tree[currIdx].backIdx;
    }
  }

  void moveToHead(Node* node){
    uint32_t nodeIdx = getIdx(node);
    if(headIdx == nodeIdx){
      return;
    }
    if(tailIdx == nodeIdx){
      tailIdx = node->forwardIdx;
    }
    if(node->backIdx != UINT32_MAX){
      tree[node->backIdx].forwardIdx = node->forwardIdx;
    }
    if(node->forwardIdx != UINT32_MAX){
      tree[node->forwardIdx].backIdx = node->backIdx;
    }
    tree[headIdx].forwardIdx = nodeIdx;
    node->backIdx = headIdx;
    node->forwardIdx = UINT32_MAX;
    headIdx = nodeIdx;
  }

  Node* push_back(const Node& node){
    if(sizeLimit != 0 && currSize >= sizeLimit){
      assert(tailIdx != UINT32_MAX);
      uint32_t currTailIdx = tailIdx;
      Node* currTail = &tree[currTailIdx];
      for(int i=0; i<currTail->children.size(); i++){
        currSize -= sizeof(Edge);
        if(currTail->children[i].childIdx != UINT32_MAX){
          tree[currTail->children[i].childIdx].parentIdx = UINT32_MAX;
        }
      }
      if(currTail->parentIdx != UINT32_MAX){
        //Update the 16th bit in the chess::Move to indicate that the child was pruned
        tree[currTail->parentIdx].children[currTail->index].value = currTail->avgValue;
        tree[currTail->parentIdx].children[currTail->index].edge.value |= 1 << 15;
        tree[currTail->parentIdx].children[currTail->index].childIdx = UINT32_MAX;
      }
      if(currTail->forwardIdx != UINT32_MAX){
        tree[currTail->forwardIdx].backIdx = UINT32_MAX;
      }

      tailIdx = currTail->forwardIdx;
      tree[tailIdx].backIdx = UINT32_MAX;

      *currTail = node;
      currTail->children.shrink_to_fit(); //Free Memory
      tree[headIdx].forwardIdx = currTailIdx;
      currTail->backIdx = headIdx;
      currTail->forwardIdx = UINT32_MAX;
      headIdx = currTailIdx;
      return &tree[headIdx];
    }
    tree.push_back(node);
    currSize += sizeof(Node);
    uint32_t newNodeIdx = uint32_t(tree.size() - 1);
    tree[newNodeIdx].backIdx = headIdx;
    if(headIdx != UINT32_MAX){
      tree[headIdx].forwardIdx = newNodeIdx;
    }
    else{
      tailIdx = newNodeIdx;
    }
    headIdx = newNodeIdx;
    return &tree[newNodeIdx];
  }
};

inline Edge findBestQEdge(Node* parent){
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

inline Node* findBestQChild(Node* parent, Tree& tree){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
  Node* currBestMove = parent->children[0].childIdx == UINT32_MAX ? nullptr : tree.getNode(parent->children[0].childIdx);

  for(int i=0; i<parent->children.size(); i++){
    if(parent->children[i].value < currBestValue){
      currBestValue = parent->children[i].value;
      currBestMove = parent->children[i].childIdx == UINT32_MAX ? nullptr : tree.getNode(parent->children[i].childIdx);
    }
  }

  return currBestMove;
}

inline float findBestQ(Node* parent){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective

  for(int i=0; i<parent->children.size(); i++){
    currBestValue = std::min(currBestValue, parent->children[i].value);
  }

  return currBestValue;
}

inline Edge findBestAEdge(Node* parent, Tree& tree){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
  Edge currBestMove = parent->children[0];

  for(int i=0; i<parent->children.size(); i++){
    float currVal = parent->children[i].childIdx != UINT32_MAX ? tree.getNode(parent->children[i].childIdx)->avgValue : parent->children[i].value;
    if(currVal < currBestValue){
      currBestValue = currVal;
      currBestMove = parent->children[i];
    }
  }

  return currBestMove;
}

inline Edge findBestEdge(Node* parent){
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

inline Node* findBestChild(Node* parent, Tree& tree){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
  Node* currBestMove = tree.getNode(parent->children[0].childIdx);

  for(int i=0; i<parent->children.size(); i++){
    if(parent->children[i].value < currBestValue){
      currBestValue = parent->children[i].value;
      currBestMove = tree.getNode(parent->children[i].childIdx);
    }
  }

  return currBestMove;
}

inline float findBestValue(Node* parent){
  float currBestValue = 2; //We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective

  for(int i=0; i<parent->children.size(); i++){
    currBestValue = std::min(currBestValue, parent->children[i].value);
  }

  return currBestValue;
}


inline void destroyTree(Tree& tree){
  tree.TT.clear();
  tree.tree.clear();
  tree.rootIdx = UINT32_MAX;
  tree.tailIdx = UINT32_MAX;
  tree.headIdx = UINT32_MAX;
  tree.currSize = 0;
}

inline uint64_t markSubtree(Tree& tree, Node* node, bool isSubtreeRoot = true, bool unmarked = true){
  uint64_t markedNodes = 0;

  if(isSubtreeRoot){
    unmarked = node->mark;
  }
  if(node){
    node->mark = !unmarked;
    markedNodes++;
    for(int i=0; i<node->children.size(); i++){
      markedNodes += markSubtree(tree, tree.getNode(node->children[i].childIdx), false, unmarked);
    }
  }
  return markedNodes;
}

//Returns the index of the new root
inline uint32_t moveRootToChild(Tree& tree, uint32_t newRootIdx){
  //LISP 2 Garbage Collection Algorithm (https://en.wikipedia.org/wiki/Mark%E2%80%93compact_algorithm#LISP_2_algorithm)
  //Mark all nodes which we want to keep
  uint64_t markedNodes = markSubtree(tree, tree.getNode(newRootIdx));
  bool marked = tree.tree[newRootIdx].mark;

  std::vector<uint32_t> forwardingTable(tree.tree.size(), UINT32_MAX);
  uint32_t freePointer = 0;

  //Reserve addresses for all nodes we want to keep
  for(uint32_t i=0; i<tree.tree.size(); i++){
    if(tree.tree[i].mark == marked){
      forwardingTable[i] = freePointer++;
    }
  }

  uint32_t newRootNewIdx = forwardingTable[newRootIdx];

  //Update LRU links to not include nodes we're about to discard
  for(uint32_t i=0; i<tree.tree.size(); i++){
    Node& node = tree.tree[i];
    if(node.mark == marked){
      uint32_t currIdx = node.backIdx;
      while(currIdx != UINT32_MAX && tree.tree[currIdx].mark == !marked){
        currIdx = tree.tree[currIdx].backIdx;
      }
      if(currIdx == UINT32_MAX){
        node.backIdx = UINT32_MAX;
        tree.tailIdx = i;
      }
      else{
        node.backIdx = currIdx;
        tree.tree[currIdx].forwardIdx = i;
      }

      currIdx = node.forwardIdx;
      while(currIdx != UINT32_MAX && tree.tree[currIdx].mark == !marked){
        currIdx = tree.tree[currIdx].forwardIdx;
      }
      if(currIdx == UINT32_MAX){
        node.forwardIdx = UINT32_MAX;
        tree.headIdx = i;
      }
      else{
        node.forwardIdx = currIdx;
        tree.tree[currIdx].backIdx = i;
      }
    }
  }

  tree.currSize = 0;

  //Update pointers to new addresses
  for(Node& node : tree.tree){
    if(node.mark == marked){
      tree.currSize += sizeof(Node);
      if(node.parentIdx != UINT32_MAX){
        node.parentIdx = forwardingTable[node.parentIdx];
      }
      if(node.backIdx != UINT32_MAX){
        node.backIdx = forwardingTable[node.backIdx];
      }
      if(node.forwardIdx != UINT32_MAX){
        node.forwardIdx = forwardingTable[node.forwardIdx];
      }
      for(int i=0; i<node.children.size(); i++){
        tree.currSize += sizeof(Edge);
        if(node.children[i].childIdx != UINT32_MAX){
          node.children[i].childIdx = forwardingTable[node.children[i].childIdx];
        }
      }
    }
  }

  tree.headIdx = forwardingTable[tree.headIdx];
  tree.tailIdx = forwardingTable[tree.tailIdx];

  //Move nodes to new addresses
  for(uint32_t i=0; i<tree.tree.size(); i++){
    if(tree.tree[i].mark == marked){
      tree.tree[forwardingTable[i]] = tree.tree[i];
    }
  }

  tree.tree.resize(markedNodes);

  return newRootNewIdx;
}

inline uint8_t selectEdge(Node* parent, Tree& tree, bool isRoot){
  float maxPriority = -2;
  uint8_t maxPriorityNodeIndex = 0;

  const float parentVisitsTerm = (isRoot ? Aurora::rootExplorationFactor.value : Aurora::explorationFactor.value)*std::log(parent->visits)*std::sqrt(std::log(parent->visits));

  float varianceScale = 
    ((1.0 / parent->iters) * 1.0) +
    ((1.0 - 1.0 / parent->iters) *
    std::clamp<double>(
      1.0 + (Aurora::varianceScaleMultiplier.value *
            (std::sqrt(std::max(parent->variance(), float(0))) - Aurora::varianceScaleOffset.value)),
      Aurora::varianceScaleMin.value,
      Aurora::varianceScaleMax.value
    ));
  
  // std::cout << std::clamp(1.0+32*(std::sqrt(std::max(parent->variance(), float(0)))-0.00625), 0.2, 2.0) << " ";

  for(int i=0; i<parent->children.size(); i++){
    Node* currNode = tree.getNode(parent->children[i].childIdx);
    Edge currEdge = parent->children[i];

    //We can make a guess about how many visits a node had before it was pruned by LRU
    bool isLRUPruned = parent->children[i].edge.value & (1 << 15);

    float childVisits = currNode ? currNode->visits : 1;
    float boostTerm = 1.0 + ((Aurora::visitBoostMultiplier.value * (parent->visits * Aurora::visitBoostOffset.value)) / 
                      (parent->visits * Aurora::visitBoostOffset.value + childVisits));

    float currPriority = -(currNode ? currNode->avgValue : currEdge.value) +
      ((boostTerm *
      varianceScale *
      parentVisitsTerm) / std::sqrt(currNode ? currNode->visits : (isLRUPruned ? Aurora::lruPrunedVisitsEstimate.value : 1)));

    assert(currPriority>=-1);

    if(currPriority>maxPriority){
      maxPriority = currPriority;
      maxPriorityNodeIndex = i;
    }
  }

  return maxPriorityNodeIndex;
}

inline void expand(Tree& tree, Node* parent, chess::MoveList& moves){
  if(moves.size()==0){return;}

  parent->children.resize(moves.size());
  tree.currSize += moves.size() * sizeof(Edge);

  for(int i=0; i<moves.size(); i++){
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

inline void backpropagate(Tree& tree, float result, std::vector<std::pair<Edge*, U64>>& edges, uint8_t visits, bool forceResult, bool runFindBestMove, bool continueBackprop){
  //Backpropagate results
  if(edges.size() == 0){return;}

  std::pair<Edge*, U64> p = edges.back();
  Edge* currEdge = p.first; U64 hash = p.second;
  edges.pop_back();

  tree.getNode(currEdge->childIdx)->visits += visits;

  float oldCurrNodeValue = 2;

  //We only need to backpropagate two types of results here: the current best child becomes worse, or there is a new best child
  if(continueBackprop){
    //If currEdge is the best move and is backpropagated to become worse, we need to run findBestQ for the parent of currEdge
    oldCurrNodeValue = 2;
    if(tree.getNode(currEdge->childIdx)->parentIdx != UINT32_MAX && edges.size() > 0 && -currEdge->value == edges.back().first->value){oldCurrNodeValue = currEdge->value;}

    //If the result is worse than the current value, there is no point in continuing the backpropagation, other than to add visits to the nodes
    if(result <= currEdge->value && !runFindBestMove && !forceResult){
      continueBackprop = false;

        tree.getNode(currEdge->childIdx)->iters++;
        float newValWeight = std::clamp(1.0/tree.getNode(currEdge->childIdx)->iters, double(Aurora::valSameMinWeight.value), 1.0);
        tree.getNode(currEdge->childIdx)->avgValue = tree.getNode(currEdge->childIdx)->avgValue*(1-newValWeight) + currEdge->value*newValWeight;
        tree.getNode(currEdge->childIdx)->sumSquaredVals = tree.getNode(currEdge->childIdx)->sumSquaredVals*(1-newValWeight) + currEdge->value*currEdge->value*newValWeight;

      TTEntry* entry = tree.getTTEntry(hash);
      entry->hash = hash >> 32;
      entry->val = currEdge->value;

      backpropagate(tree, result, edges, visits, false, runFindBestMove, continueBackprop);
      return;
    }

    currEdge->value = runFindBestMove ? -findBestQ(tree.getNode(currEdge->childIdx)) : result;

    assert(-1<=currEdge->value && 1>=currEdge->value);

    runFindBestMove = currEdge->value > oldCurrNodeValue; //currEdge(which used to be the best child)'s value got worse from currEdge's parent's perspective

    result = -currEdge->value;

    tree.getNode(currEdge->childIdx)->iters++;
    float newValWeight = std::clamp(1.0/tree.getNode(currEdge->childIdx)->iters, double(Aurora::valChangedMinWeight.value), 1.0);
    tree.getNode(currEdge->childIdx)->avgValue = tree.getNode(currEdge->childIdx)->avgValue*(1-newValWeight) + currEdge->value*newValWeight;
    tree.getNode(currEdge->childIdx)->sumSquaredVals = tree.getNode(currEdge->childIdx)->sumSquaredVals*(1-newValWeight) + currEdge->value*currEdge->value*newValWeight;
  }
  else{
    tree.getNode(currEdge->childIdx)->iters++;
    float newValWeight = std::clamp(1.0/tree.getNode(currEdge->childIdx)->iters, double(Aurora::valSameMinWeight.value), 1.0);
    tree.getNode(currEdge->childIdx)->avgValue = tree.getNode(currEdge->childIdx)->avgValue*(1-newValWeight) + currEdge->value*newValWeight;
    tree.getNode(currEdge->childIdx)->sumSquaredVals = tree.getNode(currEdge->childIdx)->sumSquaredVals*(1-newValWeight) + currEdge->value*currEdge->value*newValWeight;
  }

  TTEntry* entry = tree.getTTEntry(hash);
  entry->hash = hash >> 32;
  entry->val = currEdge->value;

  backpropagate(tree, result, edges, visits, false, runFindBestMove, continueBackprop);
}

inline void printSearchInfo(Tree& tree, std::chrono::steady_clock::time_point start, bool finalResult){
  Node* root = tree.root();
  if(Aurora::outputLevel.value >= 3){
    std::cout << "NODES: " << root->visits;
    std::cout << " SELDEPTH: " << int(tree.seldepth) <<"\n";

    std::cout.precision(5);

    std::cout << std::left
              << std::setw(8) << "Move"
              << std::setw(12) << "Q"
              << std::setw(12) << "A"
              << std::setw(12) << "I"
              << std::setw(12) << "N"
              << std::setw(12) << "V"
              << "PV" << std::endl;

    std::cout << std::string(80, '-') << std::endl;

    std::vector<Edge> sortedEdges;
    sortedEdges.reserve(root->children.size());
    for(const auto& edge : root->children) {
      sortedEdges.push_back(edge);
    }

    std::sort(sortedEdges.begin(), sortedEdges.end(), 
        [](const Edge& a, const Edge& b) {
          return a.value < b.value;
        });

    for(int i = 0; i < sortedEdges.size(); i++) {
        Edge currEdge = sortedEdges[i];
        Node* currNode = tree.getNode(currEdge.childIdx);

        std::cout << std::left
                  << std::setw(8) << currEdge.edge.toStringRep()
                  << std::setw(12) << -currEdge.value
                  << std::setw(12) << -(currNode ? currNode->avgValue : -2)
                  << std::setw(12) << (currNode ? currNode->iters : 0)
                  << std::setw(12) << (currNode ? currNode->visits : 1)
                  << std::setw(12) << (currNode ? std::sqrt(currNode->variance()) : -1);
        
        // Print PV sequence
        Node* pvNode = currNode;
        while(pvNode && pvNode->children.size() > 0) {
            Edge pvEdge = findBestEdge(pvNode);
            std::cout << pvEdge.edge.toStringRep() << " ";
            pvNode = tree.getNode(pvEdge.childIdx);
        }
        std::cout << std::endl;
    }

    std::cout.precision(10);
  }

  if(Aurora::outputLevel.value >= 2 || (finalResult && Aurora::outputLevel.value >= 1)){
    std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - start;

    std::cout <<
    "info depth " << (root->visits == tree.startNodes ? 0 : int(tree.depth / (root->visits - tree.startNodes))) <<
    " seldepth " << int(tree.seldepth) <<
    " nodes " << root->visits <<
    " score cp " << evaluation::valToCp(-findBestQ(root)) <<
    " hashfull " << int(tree.getHashfull()*1000) <<
    " ttfull " << int(tree.getTTfull()*1000) <<
    " treefull " << int(tree.getTreefull()*1000) <<
    " nps " << std::round((root->visits-tree.previousVisits)/(elapsed.count()-tree.previousElapsed)) <<
    " time " << std::round(elapsed.count()*1000) <<
    " pv ";
    Node* pvNode = root;
    while(pvNode && pvNode->children.size() > 0){
      Edge pvEdge = findBestQEdge(pvNode);
      std::cout << pvEdge.edge.toStringRep() << " ";
      pvNode = tree.getNode(pvEdge.childIdx);
    }
    std::cout << std::endl;

    tree.previousVisits = root->visits; tree.previousElapsed = elapsed.count();
  }
}

//Code relating to the time manager
enum timeManagementType: uint8_t{
  FOREVER,
  TIME,
  NODES,
  ITERS
};

struct timeManagement{
  timeManagementType tmType = FOREVER;
  float hardLimit = 0;
  float limit = 0; //For FOREVER, this does not matter. For Nodes, this is the amount of nodes. For Time, it is the amount of seconds
  bool useSoftHardNodeLimits = false;
  timeManagement(timeManagementType _tmType, uint32_t _limit = 0): tmType(_tmType), hardLimit(_limit), limit(_limit) {}
  timeManagement() {}
};

//The main search function
inline void search(chess::Board& rootBoard, timeManagement tm, Tree& tree){
  auto start = std::chrono::steady_clock::now();

  tree.setHash();
  if(Aurora::outputLevel.value >= 1){
    std::cout << "info string starting search with max tree size " <<
              (tree.sizeLimit == 0 ? "unlimited" : std::to_string(tree.sizeLimit/1000000.0)) << " mb "
              << "and TT size " <<
              (tree.TT.size()*sizeof(TTEntry)/1000000.0) << " mb"
              << std::endl;
    if(tree.TT.size() == 1){
      std::cout << "info string WARNING: TT is disabled, set either TTHash or Hash option to a non-zero value to enable" << std::endl;
    }
  }

  if(tree.rootIdx == UINT32_MAX){tree.push_back(Node()); tree.rootIdx = uint32_t(tree.tree.size()-1);}

  tree.seldepth = 0;
  tree.depth = 0;

  evaluation::NNUE<evaluation::NNUEhiddenNeurons> nnue(evaluation::nnueParameters);

  Node* currNode = tree.root();
  
  //For Printing Search Info
  int lastNodeCheck = 1;
  std::chrono::duration<float> elapsed = std::chrono::duration<float>::zero();
  tree.previousVisits = tree.root()->visits;
  tree.previousElapsed = 0;

  if(chess::getGameStatus(rootBoard, chess::isLegalMoves(rootBoard)) != chess::ONGOING){
    if(Aurora::outputLevel.value >= 0){
      std::cout << "bestmove a1a1" << std::endl;
    }
    return;
  }

  //For Time Management
  tree.startNodes = tree.root()->visits;
  int bestMoveChanges = 0;
  float bestMoveChangesMultiplier = 1;
  chess::Move currBestMove;

  //First, Check TBs
  chess::Move tbMove = chess::probeDtzTb(rootBoard);
  if(tbMove.value){
    chess::gameStatus result = chess::probeWdlTb(rootBoard);
    chess::MoveList moves(rootBoard);
    expand(tree, tree.root(), moves);
    for(int i=0; i<moves.size(); i++){
      if(moves[i] == tbMove){
        tree.root()->children[i].value = -result+0.001;
      }
      else{
        tree.root()->children[i].value = 1;
      }
    }
    tree.root()->visits = 1;
    tm.tmType = NODES;
    tm.limit = -1;
  }

  while((tm.tmType == FOREVER) ||
        (tm.tmType == TIME &&
          ((tm.useSoftHardNodeLimits && elapsed.count()<std::min(tm.limit*bestMoveChangesMultiplier, tm.hardLimit)) ||
          (!tm.useSoftHardNodeLimits && elapsed.count()<tm.limit))
        ) ||
        (tm.tmType == NODES &&
          ((tm.useSoftHardNodeLimits && (tree.root()->visits - tree.startNodes) < std::min(tm.limit*bestMoveChangesMultiplier, tm.hardLimit)) ||
          (!tm.useSoftHardNodeLimits && (tree.root()->visits - tree.startNodes) < tm.limit))
        ) ||
        (tm.tmType == ITERS &&
          ((tm.useSoftHardNodeLimits && tree.root()->iters < std::min(tm.limit*bestMoveChangesMultiplier, tm.hardLimit)) ||
          (!tm.useSoftHardNodeLimits && tree.root()->iters < tm.limit))
        )
      ){
    chess::Board board = rootBoard;

    int currDepth = 0;
    currNode = tree.root(); tree.moveToHead(tree.root());
    Edge* currEdge = nullptr;
    std::vector<std::pair<Edge*, U64>> traversePath;

    //Traverse the search tree
    while(currNode->children.size() > 0){
      currDepth++;
      
      //Move all children nodes to the front of LRU
      for(int i=0; i<currNode->children.size(); i++){
        if(currNode->children[i].childIdx != UINT32_MAX){
          tree.moveToHead(tree.getNode(currNode->children[i].childIdx));
        }
      }

      //Select Child Node to explore
      uint8_t currEdgeIndex = selectEdge(currNode, tree, currNode == tree.root());

      currEdge = &currNode->children[currEdgeIndex];
      chess::makeMove(board, currEdge->edge);
      traversePath.push_back({currEdge, board.history[board.halfmoveClock]});

      //If we only had a child edge before, create the corresponding child node
      if(currEdge->childIdx == UINT32_MAX){
        uint32_t currNodeIdx = tree.getIdx(currNode);
        bool currNodeMark = currNode->mark;
        currEdge->childIdx = tree.getIdx(tree.push_back(Node(currNodeIdx)));
        Node* childNode = tree.getNode(currEdge->childIdx);
        childNode->index = currEdgeIndex;
        childNode->mark = currNodeMark;
        childNode->visits = 1;
        childNode->iters = 1;
        childNode->avgValue = currEdge->value;
        childNode->sumSquaredVals = currEdge->value*currEdge->value;
      }

      currNode = tree.getNode(currEdge->childIdx);
    }

    //Expand & Backpropagate new values
    if(currNode->isTerminal){
      tree.depth += currDepth;
      tree.root()->visits += 1;
      tree.root()->iters += 1;
      backpropagate(tree, currEdge->value, traversePath, 1, true, false, true);
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
      std::array<std::array<int16_t, evaluation::NNUEhiddenNeurons>, 2> currAccumulator = nnue.accumulator;

      for(int i=0; i<parentNode->children.size(); i++){
        currEdge = &parentNode->children[i];

        chess::Board movedBoard = board;

        nnue.accumulator = currAccumulator;
        nnue.updateAccumulator(movedBoard, currEdge->edge);

        currEdge->value = playout(tree, movedBoard, nnue);
        assert(-1<=currEdge->value && 1>=currEdge->value);
        
        currBestValue = std::min(currBestValue, currEdge->value);
      }

      int visits = 0;
      for(int i=0; i<parentNode->children.size(); i++){
        if(parentNode->children[i].value <= currBestValue + Aurora::visitWindow.value){
          visits++;
        }
      }
      assert(visits >= 1);

      tree.depth += currDepth*visits;

      //Update root stats, since backpropagation doesn't reach the root
      tree.root()->visits += visits;
      tree.root()->iters += 1;

      //Backpropagate best value
      backpropagate(tree, -currBestValue, traversePath, visits, true, false, true);
    }

    tree.seldepth = std::max(currDepth, int(tree.seldepth));

    //Output some information on the search occasionally
    elapsed = std::chrono::steady_clock::now() - start;
    if(elapsed.count() >= lastNodeCheck*2){
      lastNodeCheck++;
      printSearchInfo(tree, start, false);
    }

    //Decide if we want to search longer or shorter depending on how much the best move has changed
    if(tm.useSoftHardNodeLimits){
      if(findBestQEdge(tree.root()).edge.value != currBestMove.value){
        bestMoveChanges++;
        currBestMove = findBestQEdge(tree.root()).edge;
      }

    double expectedBestMoveChanges =
      Aurora::bestMoveChangesCoefficient.value *
      (std::pow(tree.root()->visits, Aurora::bestMoveChangesExponent.value) -
       std::pow(tree.startNodes, Aurora::bestMoveChangesExponent.value));
    const double bestMoveChangesMultiplierMin =
      std::min(double(Aurora::bestMoveChangesMultiplierMin.value),
              double(Aurora::bestMoveChangesMultiplierMax.value));
    const double bestMoveChangesMultiplierMax =
      std::max(double(Aurora::bestMoveChangesMultiplierMin.value),
              double(Aurora::bestMoveChangesMultiplierMax.value));
    bestMoveChangesMultiplier =
      std::clamp(bestMoveChanges / expectedBestMoveChanges,
                bestMoveChangesMultiplierMin,
                bestMoveChangesMultiplierMax);
    }
  }

  //Output the final result of the search
  printSearchInfo(tree, start, true);
  if(Aurora::outputLevel.value >= 0){
    std::cout << "\nbestmove " << findBestAEdge(tree.root(), tree).edge.toStringRep() << std::endl; //std::endl to flush
  }
}

//Same as chess::makeMove except we move the root so we can keep nodes from an earlier search
//Parameter "board" must be different than parameter "rootBoard"
inline void makeMove(chess::Board& board, chess::Move move, chess::Board& rootBoard, Tree& tree){
  if(tree.rootIdx == UINT32_MAX ||
    board.equivalentHistory(rootBoard) == false
  ){
      chess::makeMove(board, move);
      return;
  }

  chess::makeMove(board, move);

  Edge newRootEdge = Edge(chess::Move());
  for(int i=0; i<tree.root()->children.size(); i++){
    if(tree.root()->children[i].edge == move){
      newRootEdge = tree.root()->children[i];
      break;
    }
  }
  uint32_t newRootIdx = newRootEdge.childIdx;

  if(newRootIdx == UINT32_MAX){tree.rootIdx = UINT32_MAX; destroyTree(tree); return;}

  tree.rootIdx = moveRootToChild(tree, newRootIdx);

  tree.root()->parentIdx = UINT32_MAX;
  tree.root()->visits--;//Visits needs to be subtracted by 1 to remove the visit which added the node
  tree.root()->iters--;//Same logic for iters

  chess::makeMove(rootBoard, move);
}

}//namespace search