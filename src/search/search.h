#ifndef PVS_DEFINED
#define PVS_DEFINED

#define WIN_VALUE (1000000)

#include "stones.h"
#include "hash.h"
#include "moves.h"
#include "../const.h"
#include "../ai/eval.h"

typedef struct SearchTree
{
    HashTable *table;
    Evaluator eval[1];
    Stones stones[1];
    unsigned char depth;
    unsigned char midDepth;
    unsigned char endDepth;
    unsigned char orderDepth;
    unsigned char hashDepth;
    unsigned char useHash;

    /* For Stats */
    size_t nodeCount;
    size_t nbCut;
    double usedTime;
    score_t score;
    uint8 isEndSearch;
} SearchTree;

typedef score_t (*SearchFunc_t)(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed);

void InitTree(SearchTree *tree, unsigned char midDepth, unsigned char endDepth, unsigned char orderDepth, unsigned char useHash, unsigned char hashDepth);
void DeleteTree(SearchTree *tree);
void ConfigTree(SearchTree *tree, unsigned char midDepth, unsigned char endDepth);
void ResetTree(SearchTree *tree);

void SearchPassMid(SearchTree *tree);
void SearchUpdateMid(SearchTree *tree, Move *move);
void SearchRestoreMid(SearchTree *tree, Move *move);
void SearchUpdateMidDeep(SearchTree *tree, uint64_t pos);
void SearchRestoreMidDeep(SearchTree *tree, uint64_t pos);

void SearchPassEnd(SearchTree *tree);
void SearchUpdateEnd(SearchTree *tree, Move *move);
void SearchRestoreEnd(SearchTree *tree, Move *move);
void SearchUpdateEndDeep(SearchTree *tree, uint64_t pos);
void SearchRestoreEndDeep(SearchTree *tree, uint64_t pos);

uint8 Search(SearchTree *tree, uint64_t own, uint64_t opp, uint8 choiceSecond);

#endif