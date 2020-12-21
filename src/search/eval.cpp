#include "eval.h"

float EvalPosTable(uint64 own, uint64 opp)
{
    int i = 0;
    float score = 0;
    for (i = 0; i < Const::BOARD_SIZE * Const::BOARD_SIZE; i++)
    {
        score += ((own >> i) & 1) * VALUE_TABLE[i];
        score -= ((opp >> i) & 1) * VALUE_TABLE[i];
    }
    return score;
}