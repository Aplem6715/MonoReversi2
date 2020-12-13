
#ifndef BIT_OPERATION_DEFINED
#define BIT_OPERATION_DEFINED

#include "const.h"

uint64 CalcMobility(uint64 aly, uint64 opp);
uint64 CalcFlip(uint64 aly, uint64 opp, uint64 pos);
int CountBits(uint64 stone);
int CalcPosIndex(uint64 pos);
uint64 CalcPosBit(unsigned char posIdx);

inline unsigned char AntiColor(unsigned char color) { return color ^ 1; }
inline uint64 GetLSB(uint64 bits) { return (-bits & bits); }
inline int max(int x1, int x2) { return x1 > x2 ? x1 : x2; }
inline int min(int x1, int x2) { return x1 < x2 ? x1 : x2; }

#endif