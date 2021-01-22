
#ifndef BIT_OPERATION_DEFINED
#define BIT_OPERATION_DEFINED

#include "stones.h"
#include "const.h"

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

inline uint64_t CalcMobility64(const uint64_t aly, const uint64_t opp);
uint64_t CalcMobility(const Stones *stones);

inline uint64_t CalcFlip64(const uint64_t own, const uint64_t opp, const uint8 pos);
uint64_t CalcFlip(const Stones *stones, const uint8 pos);

uint8 CountBits(uint64_t stone);

uint8 CalcPosIndex(uint64_t pos);
uint8 CalcPosIndex(const char *ascii);
uint64_t CalcPosBit(uint8 posIdx);
void CalcPosAscii(uint8 posIdx, char &x, int &y);

inline unsigned char AntiColor(unsigned char color)
{
    return color ^ 1;
}

#pragma warning(push)
#pragma warning(disable : 4146)
inline uint64_t GetLSB(uint64_t bits)
{
    return (-bits & bits);
}
inline uint8 NextIndex(uint64_t *bits)
{
    *bits &= *bits - 1;
    return CalcPosIndex(*bits);
}
#pragma warning(pop)

#endif