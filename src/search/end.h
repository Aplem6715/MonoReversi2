#if !defined(END_H_)
#define END_H_

#include "search.h"

score_t EndAlphaBetaDeep(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed);
score_t EndAlphaBeta(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed);

#endif // END_H_
