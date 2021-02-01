#if !defined(_MPC_H_)
#define _MPC_H_

#include "../const.h"

#define MPC_RAW_FILE "./resources/mpc_raw.csv"

#define MPC_SHALLOW_MIN 1
#define MPC_SHALLOW_MAX 6
#define MPC_DEEP_MIN 3
#define MPC_DEEP_MAX 14
#define MPC_NB_TRY 2

typedef struct MPCPair
{
    uint8 shallowDepth;
    double slope;
    double bias;
    double sigma;
} MPCPair;

extern const MPCPair mpcPairs[60][MPC_DEEP_MAX][MPC_NB_TRY];

#endif // _MPC_H_
