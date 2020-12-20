
#include "search.h"
#include "bit_operation.h"

inline void MobilityToChilds(SearchTree *tree, AbNode *node, const uint64 own, const uint64 opp, uint64 mob, const float alpha, const float beta, const unsigned char depth)
{
    uint64 pos, flip;
    AbNode *newNode;
    
    int childCount = 0;
    while (mob != 0)
    {
        pos = GetLSB(mob);
        mob ^= pos;
        flip = CalcFlip(own, opp, pos);

        newNode = GetNewABNode(&tree->nodePool);
        newNode->alpha = alpha;
        newNode->beta = beta;
        newNode->depth = depth - 1;
        newNode->own = opp ^ pos;
        newNode->opp = own ^ pos ^ flip;
        newNode->childs[0] = NULL;
        node->childs[childCount] = newNode;

        childCount++;
    }
}

void PVSRoot(uint64 own, uint64 opp, unsigned char depth)
{
    AbPool abPool;
    InitABPool(&abPool, 1000000, 100000);
}

void PVS(SearchTree *tree, AbNode *node);
