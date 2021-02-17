#if !defined(MID_H_)
#define MID_H_

#include <stdio.h>
#include "../const.h"

struct SearchTree;
typedef struct SearchTree SearchTree;

score_t MidAlphaBetaDeep(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed);
score_t MidAlphaBeta(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed);
score_t MidPVS(SearchTree *tree, const score_t alpha, const score_t beta, const unsigned char depth, const unsigned char passed);
uint8 MidRoot(SearchTree *tree, bool choiceSecond);
uint8 MidRootWithMpcLog(SearchTree *deepTree, SearchTree *shallowTree, FILE *logFile, int matchIdx, uint8 shallow, uint8 deep, uint8 minimumDepth);

#endif // MID_H_
