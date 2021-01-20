#if !defined(MID_H_)
#define MID_H_

#include "search.h"

score_t MidAlphaBetaDeep(SearchTree *tree, uint64_t own, uint64_t opp, score_t alpha, score_t beta, unsigned char depth, unsigned char passed);
score_t MidAlphaBeta(SearchTree *tree, uint64_t own, uint64_t opp, score_t alpha, score_t beta, unsigned char depth, unsigned char passed);

#endif // MID_H_
