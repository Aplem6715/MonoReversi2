#ifndef EVAL_DEFINED
#define EVAL_DEFINED

#include "../const.h"

const float VALUE_TABLE[] = {
    120, -20, 20, 5, 5, 20, -20, 120,   //
    -20, -40, -5, -5, -5, -5, -40, -20, //
    20, -5, 15, 3, 3, 15, -5, 20,       //
    5, -5, 3, 3, 3, 3, -5, 5,           //
    5, -5, 3, 3, 3, 3, -5, 5,           //
    20, -5, 15, 3, 3, 15, -5, 20,       //
    -20, -40, -5, -5, -5, -5, -40, -20, //
    120, -20, 20, 5, 5, 20, -20, 120,   //
};

float EvalPosTable(uint64 own, uint64 opp);

#endif