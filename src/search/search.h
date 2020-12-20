#ifndef PVS_DEFINED
#define PVS_DEFINED

#include "search/ab_node.h"

typedef struct SearchTree
{
    AbPool nodePool;
} SearchTree;

void PVSRoot(uint64 own, uint64 opp, unsigned char depth);
void PVS(SearchTree *tree, AbNode *currNode);

float AlphaBeta(SearchTree *tree, AbNode *currNode);

#endif