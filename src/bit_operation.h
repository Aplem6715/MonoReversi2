
#ifndef BIT_OPERATION_DEFINED
#define BIT_OPERATION_DEFINED

#include "const.h"
#include <vector>
#include <assert.h>
#include <stdarg.h>
#include <iostream>

#pragma warning(push)
#pragma warning(disable : 4267)
#include "tiny_dnn/util/aligned_allocator.h"
#pragma warning(pop)

uint64 CalcMobility(uint64 aly, uint64 opp);
uint64 CalcFlip(uint64 aly, uint64 opp, uint64 pos);
int CountBits(uint64 stone);
uint8 CalcPosIndex(uint64 pos);
uint64 CalcPosBit(uint8 posIdx);
void CalcPosAscii(uint8 posIdx, char &x, int &y);
void ConvertBoard(uint64 board, std::vector<std::vector<float_t, tiny_dnn::aligned_allocator<float_t, 64>>> &out);

inline unsigned char AntiColor(unsigned char color)
{
    return color ^ 1;
}

#pragma warning(push)
#pragma warning(disable : 4146)
inline uint64 GetLSB(uint64 bits)
{
    return (-bits & bits);
}
#pragma warning(pop)

inline int max(const int x1, const int x2)
{
    return x1 > x2 ? x1 : x2;
}

inline float maxf(const float x1, const float x2)
{
    return x1 > x2 ? x1 : x2;
}

inline int min(const int x1, const int x2)
{
    return x1 < x2 ? x1 : x2;
}

#endif