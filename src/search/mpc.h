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

#define MPC_NEST_MAX 4

typedef struct MPCPair
{
    uint8 shallowDepth;
    double slope;
    double bias;
    double std;
} MPCPair;

extern const MPCPair mpcPairs[60][MPC_DEEP_MAX - MPC_DEEP_MIN + 1][MPC_NB_TRY];

static const double MPC_T[MPC_NEST_MAX] = {
    1.036, // 85%
    1.405, // 92%
    2.054, // 98%
    99999, // LAST
};

// sample:130 t分布
static const double MPC_DEPTH_T[MPC_DEEP_MAX + 1] = {
    10,
    10,
    10,
    3.154, // 99.9
    2.355, // 99
    2.075, // 98
    1.897, // 6 - 97
    1.764,
    1.657,
    1.565,
    1.485,
    1.413,
    1.348, // 12 - 91
    1.288, // 13 - 90
    1.156, // 14 - 87.5
    1.041, // 15 - 85
    0.844  // 16 - 80%
};
/*
static const double MPC_DEPTH_T[MPC_DEEP_MAX + 1] = {
    10,
    10,
    10,
    3.154, // 99.9
    2.355, // 99
    2.355, // 99
    2.355, // 99
    2.355, // 99
    2.355, // 99
    2.355, // 99
    2.355, // 99
    2.355, // 99
    2.355, // 99
    2.355, // 99
    2.355, // 99
    2.355, // 99
    2.355, // 99
};
*/

/*
const double MPC_T[MPC_NEST_MAX] = {
    1.98,  // 80%
    1.98,  // 92%
    1.98,  // 98%
    99999, // LAST
};*/

#endif // _MPC_H_
