#ifndef AI_CONST_DEFINED
#define AI_CONST_DEFINED

#include "../const.h"

// Phase関連
#define NB_PHASE 15
#define NB_PUT_1PHASE 4
#define PHASE(nbEmpty) (nbEmpty / NB_PUT_1PHASE)

// パターン種類
#define NB_FEATURE_TYPES (11)
#define FEAT_NUM 46
#define NB_FEAT_COMB 892134

// 3累乗
#define POW0_0 0
#define POW3_0 1
#define POW3_1 3
#define POW3_2 9
#define POW3_3 27
#define POW3_4 81
#define POW3_5 243
#define POW3_6 729
#define POW3_7 2187
#define POW3_8 6561
#define POW3_9 19683
#define POW3_10 59049
#define POW3_11 177147

extern const uint16_t POW3_LIST[];

#endif