
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "random_util.h"

// 64bit疑似乱数生成
// https://stackoverflow.com/questions/3957252/is-there-any-way-to-compute-the-width-of-an-integer-type-at-compile-time/4589384#4589384
#define IMAX_BITS(m) ((m) / ((m) % 255 + 1) / 255 % 255 * 8 + 7 - 86 / ((m) % 255 + 12))
#define RAND_MAX_WIDTH IMAX_BITS(RAND_MAX)

static_assert((RAND_MAX & (RAND_MAX + 1u)) == 0, "RAND_MAX not a Mersenne number");

uint64_t rand64()
{
    uint64_t r = 0;
    for (int i = 0; i < 64; i += RAND_MAX_WIDTH)
    {
        r <<= RAND_MAX_WIDTH;
        r ^= (unsigned)rand();
    }
    return r;
}