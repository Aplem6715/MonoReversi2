#if !defined(MID_H_)
#define MID_H_

#include "search.h"

float MidAlphaBetaDeep(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed);
float MidAlphaBeta(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed);

#endif // MID_H_
