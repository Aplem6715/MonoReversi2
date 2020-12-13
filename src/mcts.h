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
    static double costWeight;

    bool finished;
    unsigned int n;
    double w;
    double policyProb;

    uint64 own, opp;
    std::vector<Node *> childs;
    NodePool *pool;

public:
    Node(){};
    ~Node(){};

    void Init(NodePool *pool, uint64 own, uint64 opp, double transProb);

    int GetNumVisited() { return n; }

    double Expand();
    double Ucb(unsigned int parentN);
    double Next();
    double Evaluate();
};

class NodePool
{
private:
    std::vector<Node *> pool;

public:
    NodePool(int initSize);
    ~NodePool();

    Node *GetNewNode(uint64 own, uint64 opp, double transProb);
    void RemoveNode(Node *node);
};

class MCTS
{
private:
public:
    uint64 DeterminPos(uint64 own, uint64 opp);
};

#endif