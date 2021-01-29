#if !defined(END_H_)
#define END_H_

#include "search.h"

score_t EndAlphaBetaDeep(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed);
score_t EndAlphaBeta(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed);
score_t EndPVS(SearchTree *tree, const score_t alpha, const score_t beta, unsigned char depth, unsigned char passed);
uint8 EndRoot(SearchTree *tree, uint8 choiceSecond);

#endif // END_H_
