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
    int n;
    double w;
    uint64 own, opp;
    std::vector<Node *> childs;
    NodePool *pool;

public:
    Node(){};
    ~Node(){};

    void Init(uint64 own, uint64 opp);

    int GetNumVisited() { return n; }

    void Expand();
    double Next();
    double BackProp();
    double Evaluate();
    double Ucb();
};

class NodePool
{
private:
    std::vector<Node *> pool;

public:
    NodePool(int initSize);
    ~NodePool();

    Node *GetNewNode(uint64 own, uint64 opp);
    void RemoveNode(Node *node);
};

class MCTS
{
private:
public:
    uint64 DeterminPos(uint64 own, uint64 opp);
};

#endif