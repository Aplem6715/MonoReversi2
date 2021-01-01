#ifndef GAME_RECORD_DEFINED
#define GAME_RECORD_DEFINED

#include "../const.h"
#include "../ai/eval.h"

#define RESULT_UNSETTLED -100

typedef struct GameRecord
{
    uint16 featStats[FEAT_NUM];
    uint8 nbEmpty;
    uint8 color;
    signed char resultForBlack;
} GameRecord;

#endif