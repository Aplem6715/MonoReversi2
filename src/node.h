
#ifndef NODE_DEFINED
#define NODE_DEFINED

#include <vector>
#include "const.h"
class NodePool;

class Node
{
    
private:
    static unsigned int expand_thresh;
    static float costWeight;

    bool finished;
    unsigned int n;
    float w;
    float policyProb;
    unsigned char posIdx;

    uint64 own, opp;
    std::vector<Node *> childs;
    NodePool *pool;

public:
    Node(){};
    ~Node(){};

    void Init(NodePool *pool, uint64 own, uint64 opp, unsigned char posIdx, float transProb);

    int GetNumVisited() { return n; }
    unsigned GetPos() { return posIdx; }

    float Expand();
    float Ucb(unsigned int parentN);
    float Next();
    float Evaluate();
    unsigned char SelectMove(bool isDeterministic);
};

class NodePool
{
private:
    std::vector<Node *> pool;
    int additionSize;

public:
    NodePool(int initSize, int additionSize);
    ~NodePool();

    Node *GetNewNode(uint64 own, uint64 opp, float transProb, unsigned char posIdx);
    void RemoveNode(Node *node);
};

#endif
