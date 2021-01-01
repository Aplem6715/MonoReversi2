#ifndef PVS_DEFINED
#define PVS_DEFINED

#define WIN_VALUE (1000000)

#include <chrono>
#include "hash.h"
#include "../const.h"
#include "../ai/eval.h"

typedef struct SearchTree
{
    HashTable *table;
    Evaluator eval[1];
    size_t nodeCount;
    unsigned char depth;
    unsigned char orderDepth;
    unsigned char hashDepth;
    unsigned char useHash;
    unsigned char nbEmpty;
    double usedTime;
    float score;
    // 外部ライブラリを使用して評価を行うか（大会で外部ライブラリの使用が禁止されている場合用）
    bool useTinyDNN;
} SearchTree;

typedef float (*SearchFunc_t)(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed);

void InitTree(SearchTree *tree, unsigned char depth, unsigned char orderDepth, unsigned char useHash, unsigned char hashDepth);
void DeleteTree(SearchTree *tree);
void ConfigTree(SearchTree *tree, unsigned char depth);
void ResetTree(SearchTree *tree);
uint64 Search(SearchTree *tree, uint64 own, uint64 opp);
void PVS(SearchTree *tree);

float AlphaBetaDeep(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed);
float AlphaBeta(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed);

#endif