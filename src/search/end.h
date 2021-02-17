#if !defined(END_H_)
#define END_H_

#include "../const.h"
struct SearchTree;
typedef struct SearchTree SearchTree;

score_t EndAlphaBetaDeep(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed);
score_t EndAlphaBeta(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed);
score_t EndPVS(SearchTree *tree, const score_t alpha, const score_t beta, const unsigned char depth, const unsigned char passed);
uint8 EndRoot(SearchTree *tree, bool choiceSecond);

#endif // END_H_
