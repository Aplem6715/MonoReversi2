#ifndef PVS_DEFINED
#define PVS_DEFINED

#define WIN_VALUE (1000000)

#include <chrono>
#include "hash.h"
#include "../const.h"

typedef struct SearchTree
{
    HashTable *table;
    unsigned char depth;
    size_t nodeCount;
    double usedTime;
    float score;
} SearchTree;

void InitTree(SearchTree *tree, unsigned char depth);
void DeleteTree(SearchTree *tree);
void ConfigTree(SearchTree *tree, unsigned char depth);
void ResetTree(SearchTree *tree);
uint64 Search(SearchTree *tree, uint64 own, uint64 opp);
void PVS(SearchTree *tree);

float AlphaBeta(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed);

#endif