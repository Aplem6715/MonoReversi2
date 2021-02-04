#if !defined(_MPC_H_)
#define _MPC_H_

#include "../const.h"
#include "search.h"

#define MPC_RAW_FILE "./resources/mpc/mpc_raw_tmp.csv"

#define MPC_SHALLOW_MIN 1
#define MPC_SHALLOW_MAX 6
#define MPC_DEEP_MIN 3
#define MPC_DEEP_MAX 16
#define MPC_NB_TRY 2

#define MPC_T 1.98

typedef struct MPCPair
{
    uint8 shallowDepth;
    double slope;
    double bias;
    double std;
} MPCPair;

extern const MPCPair mpcPairs[60][MPC_DEEP_MAX - MPC_DEEP_MIN + 1][MPC_NB_TRY];

#endif // _MPC_H_
