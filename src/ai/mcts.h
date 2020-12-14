#ifndef MCTS_DEFINED
#define MCTS_DEFINED

#include <vector>
#include <time.h>
#include "const.h"
#include "node.h"

class MCTS
{
private:
    static unsigned int NODE_PER_SEC;
    static unsigned int EXPLORE_ITER_MIN;
    NodePool *pool;
    clock_t timeLimit;

public:
    MCTS(unsigned int timeLimit);
    ~MCTS();
    uint64 DeterminPos(uint64 own, uint64 opp);
};

#endif