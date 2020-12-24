#include "hash.h"
#include "../bit_operation.h"
#include <random>
#include <stdlib.h>

#define HASH_TABLE_SIZE (1 << 24)
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

void InitHashTable(HashTable *table)
{
    table->size = HASH_TABLE_SIZE;
    table->data = (HashData *)calloc(table->size, sizeof(HashData));
    if (table->data == NULL)
    {
        printf("ハッシュテーブルのメモリ確保失敗\n");
        return;
    }
    assert(table->size % 2 == 0);
}

void FreeHashTable(HashTable *table)
{
    free(table->data);
    table->size = 0;
}

inline uint64 GetHashCode(uint64 own, uint64 opp)
{
    uint64 code;

    // own: 64bit -> 8x8bit
    const uint8 *cursor = (uint8 *)(&own);
    code = RawHash[0][cursor[0]];
    code ^= RawHash[1][cursor[1]];
    code ^= RawHash[2][cursor[2]];
    code ^= RawHash[3][cursor[3]];
    code ^= RawHash[4][cursor[4]];
    code ^= RawHash[5][cursor[5]];
    code ^= RawHash[6][cursor[6]];
    code ^= RawHash[7][cursor[7]];

    // opp: 64bit -> 8x8bit
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

HashData *GetHashData(HashTable *table, uint64 own, uint64 opp, uint8 depth, uint64 *hashCode, HashHitState *hitState)
{
    // ハッシュコード取得
    *hashCode = GetHashCode(own, opp);
    // サイズでモジュロ演算(code % size)
    uint64 index = *hashCode & (table->size - 1);
    HashData *data = &table->data[index];

    if (data->own == own && data->opp == opp)
    {
        if (data->depth == depth)
        {
            table->nbHit++;
            *hitState = HASH_HIT;
            return data;
        }
        else if (data->depth > depth)
        {
            *hitState = HASH_DEEPER;
        }
        else
        {
            *hitState = HASH_SHALLOWER;
        }
    }
    else
    {
        if (data->own == 0 && data->opp == 0)
        {
            *hitState = HASH_EMPTY;
        }
        else
        {
            *hitState = HASH_DEFFERENT;
        }
    }
    return NULL;
}

void HashOverwrite(HashTable *table, uint64 own, uint64 opp, uint8 depth, float lower, float upper, uint64 hashCode)
{
    uint64 index = hashCode & (table->size - 1);
    HashData *data = &table->data[index];

    data->own = own;
    data->opp = opp;
    data->depth = depth;
    data->lower = lower;
    data->upper = upper;
}

void HashPriorityOverwrite(HashTable *table, uint64 own, uint64 opp, uint8 depth, float lower, float upper, uint64 hashCode)
{
    uint64 index = hashCode & (table->size - 1);
    HashData *data = &table->data[index];

    if (depth > data->depth)
    {
        data->own = own;
        data->opp = opp;
        data->depth = depth;
        data->lower = lower;
        data->upper = upper;
    }
}
