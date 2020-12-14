#include <math.h>
#include <random>
#include "ai/mcts.h"
#include "ai/node.h"
#include "bit_operation.h"

unsigned int MCTS::NODE_PER_SEC = 80000;
unsigned int MCTS::EXPLORE_ITER_MIN = 10;

MCTS::MCTS(unsigned int timeLimitSec)
{
    this->timeLimit = timeLimitSec * CLOCKS_PER_SEC;
    this->pool = new NodePool(timeLimitSec * this->NODE_PER_SEC, this->NODE_PER_SEC);
}

MCTS::~MCTS()
{
    delete this->pool;
}

uint64 MCTS::DeterminPos(uint64 own, uint64 opp)
{
    clock_t start = clock();

    uint64 mobility = CalcMobility(own, opp);
    uint64 pos = 0;
    uint64 flips = 0;

    //vec_t result = net.predict;
    //result = {p, v};

    Node *node = pool->GetNewNode(own, opp, 1, 0);
    int nbSearched = 0;

    for (int i = 0; i < 10; i++)
    {
        node->Next();
        nbSearched++;
    }

    // 指定時間が経過するまで探索
    while (start + timeLimit > clock())
    {
        node->Next();
        nbSearched++;
    }
    printf("探索量:%d\n", nbSearched);

    unsigned char posIdx = node->SelectMove(true);
    return CalcPosBit(posIdx);
}