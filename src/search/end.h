#if !defined(END_H_)
#define END_H_

#include "search.h"

float EndAlphaBetaDeep(SearchTree *tree, uint64_t own, uint64_t opp, float alpha, float beta, unsigned char depth, unsigned char passed);
float EndAlphaBeta(SearchTree *tree, uint64_t own, uint64_t opp, float alpha, float beta, unsigned char depth, unsigned char passed);

#endif // END_H_
