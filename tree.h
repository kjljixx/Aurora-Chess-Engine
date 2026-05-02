#pragma once
#include "chess.h"
#include "aurora.h"
#include <vector>
#include <deque>
#include <cassert>
#include <algorithm>
#include <iostream>

namespace search {

struct Node;

#pragma pack(push, 1)
struct Edge {
    Node* child;
    float value;
    chess::Move edge;

    Edge() : child(nullptr), value(-2), edge(chess::Move()) {}
    Edge(chess::Move move) : child(nullptr), value(-2), edge(move) {}
};
#pragma pack(pop)

struct Node {
    std::vector<Edge> children;
    Node* parent;

    // For LRU tree management
    Node* backLink = nullptr; // back = node used less recently
    Node* forwardLink = nullptr; // forward = node used more recently
    // For Tree Reuse
    Node* newAddress = nullptr;

    uint32_t visits;
    int iters;
    float avgValue;
    float sumSquaredVals = 0;

    bool isTerminal;
    uint8_t index;
    // For Tree Reuse
    bool mark = false;

    Node(Node* parent) :
        parent(parent),
        visits(0), iters(0), avgValue(-2), isTerminal(false) {}

    Node() : parent(nullptr), visits(0), iters(0), avgValue(-2), isTerminal(false) {}

    float variance() {
        return (sumSquaredVals - avgValue * avgValue);
    }
};

inline Edge findBestQEdge(Node* parent) {
    float currBestValue = 2; // We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
    Edge currBestMove = parent->children[0];

    for (int i = 0; i < parent->children.size(); i++) {
        if (parent->children[i].value < currBestValue) {
            currBestValue = parent->children[i].value;
            currBestMove = parent->children[i];
        }
    }

    return currBestMove;
}

inline Node* findBestQChild(Node* parent) {
    float currBestValue = 2; // We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
    Node* currBestMove = parent->children[0].child;

    for (int i = 0; i < parent->children.size(); i++) {
        if (parent->children[i].value < currBestValue) {
            currBestValue = parent->children[i].value;
            currBestMove = parent->children[i].child;
        }
    }

    return currBestMove;
}

inline float findBestQ(Node* parent) {
    float currBestValue = 2; // We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective

    for (int i = 0; i < parent->children.size(); i++) {
        currBestValue = std::min(currBestValue, parent->children[i].value);
    }

    return currBestValue;
}

inline Edge findBestAEdge(Node* parent) {
    float currBestValue = 2; // We want to find the node with the least Q, which is the best move from the parent since Q is from the side to move's perspective
    Edge currBestMove = parent->children[0];

    for (int i = 0; i < parent->children.size(); i++) {
        float currVal = parent->children[i].child ? parent->children[i].child->avgValue : parent->children[i].value;
        if (currVal < currBestValue) {
            currBestValue = currVal;
            currBestMove = parent->children[i];
        }
    }

    return currBestMove;
}

struct TTEntry {
    float val;
    uint32_t hash;

    TTEntry() : val(-2), hash(0) {}
};

struct Tree {
    std::deque<Node> tree;
    std::vector<TTEntry> TT;
    Node* root = nullptr;
    uint64_t sizeLimit = 0;
    uint64_t currSize = 0;
    Node* tail = nullptr;
    Node* head = nullptr;

    // Used for nps calculations in printing search info
    int previousVisits = 0;
    float previousElapsed = 0;

    uint8_t seldepth = 0;
    uint32_t depth = 0;

    // Nodes at the start of a search
    uint32_t startNodes = 0;

    TTEntry* getTTEntry(U64 hash) {
        return &TT[hash % TT.size()];
    }

    void setHash() {
        uint32_t hashMb = Aurora::hash.value;
        sizeLimit = 1000000 * hashMb * (Aurora::ttHash.value ? 1 : 0.8);
        uint32_t ttHashBytes = Aurora::ttHash.value
                                  ? Aurora::ttHash.value * 1000000
                                  : hashMb * 1000000 * 0.2;
        size_t targetEntries = std::max<size_t>(1, ttHashBytes / sizeof(TTEntry));
        if (TT.size() != targetEntries) {
            TT.clear();
            TT.resize(targetEntries);
        }
    }

    float getHashfull() {
        float treeHashfull = sizeLimit > 0 ? float(currSize) / sizeLimit : 0;

        float ttHashfull = 0;
        int numTTEntriesToCheck = std::min(1000, int(TT.size()));
        for (int i = 0; i < numTTEntriesToCheck; i++) {
            if (TT[i].val != -2) {
                ttHashfull += 1;
            }
        }
        ttHashfull /= numTTEntriesToCheck;

        float totalHash = TT.size() * sizeof(TTEntry) + sizeLimit;
        return treeHashfull * (sizeLimit / totalHash) + ttHashfull * (TT.size() * sizeof(TTEntry) / totalHash);
    }

    // for debug purposes
    void passthrough() {
        Node* currNode = tail;
        while (currNode) {
            currNode = currNode->forwardLink;
        }
        currNode = head;
        while (currNode) {
            currNode = currNode->backLink;
        }
        return;
    }

    void moveToHead(Node* node) {
        if (head == node) {
            return;
        }
        if (tail == node && node->forwardLink) {
            tail = node->forwardLink;
        }
        if (node->backLink) {
            node->backLink->forwardLink = node->forwardLink;
        }
        if (node->forwardLink) {
            node->forwardLink->backLink = node->backLink;
        }
        head->forwardLink = node;
        node->backLink = head;
        node->forwardLink = nullptr;
        head = node;
    }

    Node* push_back(Node node) {
        if (sizeLimit != 0 && currSize >= sizeLimit) {
            assert(tail);
            Node* currTail = tail;
            for (int i = 0; i < currTail->children.size(); i++) {
                currSize -= sizeof(Edge);
                if (currTail->children[i].child) {
                    currTail->children[i].child->parent = nullptr;
                }
            }
            if (currTail->parent) {
                // Update the 16th bit in the chess::Move to indicate that the child was pruned
                currTail->parent->children[currTail->index].value = currTail->avgValue;
                currTail->parent->children[currTail->index].edge.value |= 1 << 15;
                currTail->parent->children[currTail->index].child = nullptr;
            }
            if (currTail->forwardLink) {
                currTail->forwardLink->backLink = nullptr;
            }

            tail = currTail->forwardLink;
            tail->backLink = nullptr;

            *currTail = node;
            currTail->children.shrink_to_fit(); // Free Memory
            head->forwardLink = currTail;
            currTail->backLink = head;
            currTail->forwardLink = nullptr;
            head = currTail;
            return head;
        }
        else {
            tree.push_back(node);
            currSize += sizeof(Node);
            tree.back().backLink = head;
            if (head) {
                head->forwardLink = &tree.back();
            }
            else {
                tail = &tree.back();
            }
            head = &tree.back();
            return &tree.back();
        }
    }
};

inline void destroyTree(Tree& tree) {
    tree.TT.clear();
    tree.tree.clear();
    tree.root = nullptr;
    tree.tail = nullptr;
    tree.head = nullptr;
    tree.currSize = 0;
}

inline uint64_t markSubtree(Node* node, bool isSubtreeRoot = true, bool unmarked = true) {
    uint64_t markedNodes = 0;

    if (isSubtreeRoot) {
        unmarked = node->mark;
    }
    if (node) {
        node->mark = !unmarked;
        markedNodes++;
        for (int i = 0; i < node->children.size(); i++) {
            markedNodes += markSubtree(node->children[i].child, false, unmarked);
        }
    }
    return markedNodes;
}

// Returns a pointer to the new root, which is different from the pointer given as a parameter because of the garbage collection
inline Node* moveRootToChild(Tree& tree, Node* newRoot, Node* currRoot) {
    // LISP 2 Garbage Collection Algorithm (https://en.wikipedia.org/wiki/Mark%E2%80%93compact_algorithm#LISP_2_algorithm)
    // Mark all nodes which we want to keep
    uint64_t markedNodes = markSubtree(newRoot);
    bool marked = newRoot->mark;

    // Index of the next unreserved address in tree.tree
    uint64_t freePointer = 0;

    // Reserve addresses for all nodes we want to keep
    for (uint32_t i = 0; i < tree.tree.size(); i++) {
        Node* livePointer = &tree.tree[i];
        if (livePointer->mark == marked) {
            livePointer->newAddress = &tree.tree[freePointer];
            freePointer++;
        }
    }

    Node* newRootNewAddress = newRoot->newAddress;

    // Update LRU links to not include nodes we're about to discard
    for (Node& node : tree.tree) {
        if (node.mark == marked) {
            Node* currNode = node.backLink;
            while (currNode && currNode->mark == !marked) {
                currNode = currNode->backLink;
            }
            if (!currNode) {
                node.backLink = nullptr;
                tree.tail = &node;
            }
            else {
                node.backLink = currNode;
                currNode->forwardLink = &node;
            }

            currNode = node.forwardLink;
            while (currNode && currNode->mark == !marked) {
                currNode = currNode->forwardLink;
            }
            if (!currNode) {
                node.forwardLink = nullptr;
                tree.head = &node;
            }
            else {
                node.forwardLink = currNode;
                currNode->backLink = &node;
            }
        }
    }

    tree.currSize = 0;

    // Update pointers to new addresses
    for (Node& node : tree.tree) {
        if (node.mark == marked) {
            tree.currSize += sizeof(Node);
            if (node.parent) {
                assert(&node == newRoot || node.parent->mark == marked);
                node.parent = node.parent->newAddress;
            }
            if (node.backLink) {
                node.backLink = node.backLink->newAddress;
            }
            if (node.forwardLink) {
                node.forwardLink = node.forwardLink->newAddress;
            }
            for (int i = 0; i < node.children.size(); i++) {
                tree.currSize += sizeof(Edge);
                if (node.children[i].child) {
                    assert(node.children[i].child->mark == marked);
                    node.children[i].child = node.children[i].child->newAddress;
                }
            }
        }
    }

    tree.head = tree.head->newAddress;
    tree.tail = tree.tail->newAddress;

    // Move nodes to new addresses
    for (uint32_t i = 0; i < tree.tree.size(); i++) {
        Node* livePointer = &tree.tree[i];
        if (livePointer->mark == marked) {
            *(livePointer->newAddress) = *livePointer;
        }
        livePointer++;
    }

    tree.tree.resize(markedNodes);

    return newRootNewAddress;
}

} // namespace search
