#include "hash.h"
#include "../bit_operation.h"
#include <random>
#include <stdlib.h>

#define HASH_TABLE_SIZE (1 << 27)
// RawHash[8行x2色][列内8石のパターン]
uint64 RawHash[8 * 2][1 << 8];

void InitHash()
{
    std::mt19937_64 mt(HASH_SEED);

    for (int i = 0; i < 8 * 2; i++)
    {
        for (int j = 0; j < 1 << 8; j++)
        {
            do
            {
                RawHash[i][j] = mt();
            } while (CountBits(RawHash[i][j]) < MIN_RAWHASH_BIT);
        }
    }
}

void InitHashTable(HashTable &table)
{
    table.size = HASH_TABLE_SIZE;
    table.data = (HashData *)malloc(sizeof(HashData) * table.size);
}

void FreeHashTable(HashTable &table)
{
    free(table.data);
    table.size = 0;
}

uint64 GetHashCode(uint64 own, uint64 opp)
{
    uint64 code;

    // 64bit -> 8x8bit
    const uint8 *cursor = (uint8 *)(&own);
    code = RawHash[0][cursor[0]];
    code ^= RawHash[1][cursor[1]];
    code ^= RawHash[2][cursor[2]];
    code ^= RawHash[3][cursor[3]];
    code ^= RawHash[4][cursor[4]];
    code ^= RawHash[5][cursor[5]];
    code ^= RawHash[6][cursor[6]];
    code ^= RawHash[7][cursor[7]];

    // 64bit -> 8x8bit
    cursor = (uint8 *)(&opp);
    code ^= RawHash[8][cursor[0]];
    code ^= RawHash[9][cursor[1]];
    code ^= RawHash[10][cursor[2]];
    code ^= RawHash[11][cursor[3]];
    code ^= RawHash[12][cursor[4]];
    code ^= RawHash[13][cursor[5]];
    code ^= RawHash[14][cursor[6]];
    code ^= RawHash[15][cursor[7]];

    return code;
}