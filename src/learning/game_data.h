#ifndef GAME_DATA_DEFINED
#define GAME_DATA_DEFINED

#include "../const.h"
#include "../ai/eval.h"

typedef struct GameData
{
    uint16 featStats[FEAT_NUM];
    uint8 selected;
    uint8 nbEmpty;
    signed char resultForBlack;
} GameData;

#endif