#if !defined(MID_H_)
#define MID_H_

#include "search.h"

score_t MidAlphaBetaDeep(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed);
score_t MidAlphaBeta(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed);
score_t MidPVS(SearchTree *tree, const score_t alpha, const score_t beta, unsigned char depth, unsigned char passed);
uint8 MidRoot(SearchTree *tree, uint8 choiceSecond);
uint8 MidRootWithMpcLog(SearchTree *tree, FILE *logFile);

#endif // MID_H_
