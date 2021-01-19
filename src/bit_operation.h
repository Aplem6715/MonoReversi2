
#ifndef BIT_OPERATION_DEFINED
#define BIT_OPERATION_DEFINED

#include "const.h"

uint64 CalcMobility(const uint64 aly, const uint64 opp);
uint64 CalcFlipOptimized(uint64 own, uint64 opp, uint8 pos);
uint8 CountBits(uint64 stone);
uint8 CalcPosIndex(uint64 pos);
uint8 CalcPosIndex(const char *ascii);
uint64 CalcPosBit(uint8 posIdx);
void CalcPosAscii(uint8 posIdx, char &x, int &y);

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
inline uint8 NextIndex(uint64 *bits)
{
    *bits &= *bits - 1;
    return CalcPosIndex(*bits);
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

inline float minf(const float x1, const float x2)
{
    return x1 < x2 ? x1 : x2;
}

#endif