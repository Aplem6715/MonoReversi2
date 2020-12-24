
#include "bit_operation.h"

uint64 CalcMobilityL(uint64 aly, uint64 masked_opp, uint64 empty,
                     unsigned char dir)
{
    uint64 tmp = masked_opp & (aly << dir);
    tmp |= masked_opp & (tmp << dir);
    tmp |= masked_opp & (tmp << dir);
    tmp |= masked_opp & (tmp << dir);
    tmp |= masked_opp & (tmp << dir);
    tmp |= masked_opp & (tmp << dir);
    return empty & (tmp << dir);
}

uint64 CalcMobilityR(uint64 aly, uint64 masked_opp, uint64 empty,
                     unsigned char dir)
{
    uint64 tmp = masked_opp & (aly >> dir);
    tmp |= masked_opp & (tmp >> dir);
    tmp |= masked_opp & (tmp >> dir);
    tmp |= masked_opp & (tmp >> dir);
    tmp |= masked_opp & (tmp >> dir);
    tmp |= masked_opp & (tmp >> dir);
    return empty & (tmp >> dir);
}

uint64 CalcMobility(uint64 aly, uint64 opp)
{
    uint64 empty = ~(aly | opp);
    // 上下左右の壁をまたがないようにマスクをかける
    uint64 mask_rl = opp & 0x7e7e7e7e7e7e7e7e;
    uint64 mask_ud = opp & 0x00ffffffffffff00;
    uint64 mask_al = opp & 0x007e7e7e7e7e7e00;

    // 8方向に対して着手可能位置を検索
    uint64 mobilty = CalcMobilityL(aly, mask_rl, empty, 1);
    mobilty |= CalcMobilityL(aly, mask_ud, empty, 8);
    mobilty |= CalcMobilityL(aly, mask_al, empty, 7);
    mobilty |= CalcMobilityL(aly, mask_al, empty, 9);
    mobilty |= CalcMobilityR(aly, mask_rl, empty, 1);
    mobilty |= CalcMobilityR(aly, mask_ud, empty, 8);
    mobilty |= CalcMobilityR(aly, mask_al, empty, 7);
    mobilty |= CalcMobilityR(aly, mask_al, empty, 9);
    return mobilty;
}

uint64 CalcFlipL(uint64 aly, uint64 masked_empty, uint64 pos,
                 unsigned char dir)
{
    uint64 rev = 0;
    uint64 tmp = ~(aly | masked_empty) & (pos << dir);
    if (tmp)
    {
        for (int i = 0; i < 6; i++)
        {
            tmp <<= dir;
            if (tmp & masked_empty)
            {
                break;
            }
            else if (tmp & aly)
            {
                rev |= tmp >> dir;
                break;
            }
            else
            {
                tmp |= tmp >> dir;
            }
        }
    }
    return rev;
}

uint64 CalcFlipR(uint64 aly, uint64 masked_empty, uint64 pos,
                 unsigned char dir)
{
    uint64 rev = 0;
    uint64 tmp = ~(aly | masked_empty) & (pos >> dir);
    if (tmp)
    {
        for (int i = 0; i < 6; i++)
        {
            tmp >>= dir;
            if (tmp & masked_empty)
            {
                break;
            }
            else if (tmp & aly)
            {
                rev |= tmp << dir;
                break;
            }
            else
            {
                tmp |= tmp << dir;
            }
        }
    }
    return rev;
}

uint64 CalcFlip(uint64 aly, uint64 opp, uint64 pos)
{
    uint64 blank_rl = ~(aly | (opp & 0x7e7e7e7e7e7e7e7e));
    uint64 blank_ud = ~(aly | (opp & 0x00ffffffffffff00));
    uint64 blank_al = ~(aly | (opp & 0x007e7e7e7e7e7e00));
    uint64 rev = CalcFlipL(aly, blank_rl, pos, 1);
    rev |= CalcFlipL(aly, blank_ud, pos, 8);
    rev |= CalcFlipL(aly, blank_al, pos, 7);
    rev |= CalcFlipL(aly, blank_al, pos, 9);
    rev |= CalcFlipR(aly, blank_rl, pos, 1);
    rev |= CalcFlipR(aly, blank_ud, pos, 8);
    rev |= CalcFlipR(aly, blank_al, pos, 7);
    rev |= CalcFlipR(aly, blank_al, pos, 9);
    return rev;
}

int CountBits(uint64 stone)
{
    int count = 0;
    while (stone)
    {
        stone &= stone - 1;
        count++;
    }
    return count;
}

uint8 CalcPosIndex(uint64 pos)
{
    uint64 cursor = 0x8000000000000000;
    int idx = 0;
    while ((pos & cursor) == 0)
    {
        cursor >>= 1;
        idx++;
    }
    return idx;
}

uint8 CalcPosIndex(const char *ascii)
{
    return (ascii[0] - 'A') + (ascii[1] - '1') * 8;
}

uint64 CalcPosBit(unsigned char posIdx)
{
    return 0x8000000000000000 >> posIdx;
}

void CalcPosAscii(unsigned char posIdx, char &x, int &y)
{
    x = 'A' + posIdx % 8;
    y = posIdx / 8 + 1;
}
