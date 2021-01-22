#if !defined(_STONES_H_)
#define _STONES_H_

#include "const.h"

typedef struct Stones
{
    uint64_t own;
    uint64_t opp;
} Stones;

inline void SwapStones(Stones *stones)
{
    uint64_t tmp = stones->opp;
    stones->opp = stones->own;
    stones->own = tmp;
}

inline void StonesUpdate(Stones *stones, uint64_t pos, uint64_t flip)
{
    stones->own ^= (flip | pos);
    stones->opp ^= flip;
    SwapStones(stones);
}

inline void StonesRestore(Stones *stones, uint64_t pos, uint64_t flip)
{
    SwapStones(stones);
    stones->own ^= (flip | pos);
    stones->opp ^= flip;
}

#endif // _STONES_H_
