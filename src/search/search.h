#ifndef PVS_DEFINED
#define PVS_DEFINED

#define WIN_VALUE (1000000)

#include "hash.h"
#include "../const.h"
#include "../ai/eval.h"

typedef struct SearchTree
{
    HashTable *table;
    Evaluator eval[1];
    size_t nodeCount;
    size_t nbCut;
    unsigned char depth;
    unsigned char midDepth;
    unsigned char endDepth;
    unsigned char orderDepth;
    unsigned char hashDepth;
    unsigned char useHash;
    double usedTime;
    float score;
    uint8 isEndSearch;
} SearchTree;

typedef float (*SearchFunc_t)(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed);

void InitTree(SearchTree *tree, unsigned char midDepth, unsigned char endDepth, unsigned char orderDepth, unsigned char useHash, unsigned char hashDepth);
void DeleteTree(SearchTree *tree);
void ConfigTree(SearchTree *tree, unsigned char midDepth, unsigned char endDepth);
void ResetTree(SearchTree *tree);
uint8 Search(SearchTree *tree, uint64 own, uint64 opp, uint8 choiceSecond);
void PVS(SearchTree *tree);

#endif