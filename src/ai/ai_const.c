#include "ai_const.h"

const uint16_t POW3_LIST[] = {POW3_0, POW3_1, POW3_2, POW3_3, POW3_4, POW3_5, POW3_6, POW3_7, POW3_8, POW3_9, POW3_10};

const uint8 FeatID2Type[FEAT_NUM] = {
    0, 0, 0, 0,     // LINE2
    1, 1, 1, 1,     // LINE3
    2, 2, 2, 2,     // LINE4
    3, 3, 3, 3,     // DIAG4
    4, 4, 4, 4,     // DIAG5
    5, 5, 5, 5,     // DIAG6
    6, 6, 6, 6,     // DIAG7
    7, 7,           // DIAG8
    8, 8, 8, 8,     // EDGEX
    9, 9, 9, 9,     // CORNR
    10, 10, 10, 10, // BOX10
    10, 10, 10, 10  // BOX10-2
};