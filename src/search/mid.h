#if !defined(MID_H_)
#define MID_H_

#include "search.h"

float MidAlphaBetaDeep(SearchTree *tree, uint64_t own, uint64_t opp, float alpha, float beta, unsigned char depth, unsigned char passed);
float MidAlphaBeta(SearchTree *tree, uint64_t own, uint64_t opp, float alpha, float beta, unsigned char depth, unsigned char passed);

#endif // MID_H_
