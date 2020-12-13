#ifndef MCTS_DEFINED
#define MCTS_DEFINED

#include <vector>
#include "const.h"

class NodePool;
class MCTS;

class Node
{
private:
    static int expand_thresh;
    static float costWeight;

    bool finished;
    unsigned int n;
    float w;
    float policyProb;

    uint64 own, opp;
    std::vector<Node *> childs;
    NodePool *pool;

public:
    Node(){};
    ~Node(){};

    void Init(NodePool *pool, uint64 own, uint64 opp, float transProb);

    int GetNumVisited() { return n; }

    float Expand();
    float Ucb(unsigned int parentN);
    float Next();
    float Evaluate();
};

class NodePool
{
private:
    std::vector<Node *> pool;

public:
    NodePool(int initSize);
    ~NodePool();

    Node *GetNewNode(uint64 own, uint64 opp, float transProb);
    void RemoveNode(Node *node);
};

class MCTS
{
private:
public:
    uint64 DeterminPos(uint64 own, uint64 opp);
};

#endif