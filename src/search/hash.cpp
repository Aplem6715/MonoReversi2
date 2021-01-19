#include "hash.h"
#include "../bit_operation.h"
#include <random>
#include <stdlib.h>
#include <assert.h>

#define HASH_TABLE_SIZE (1 << 24)
// RawHash[8行x2色][列内8石のパターン]
uint64_t RawHash[8 * 2][1 << 8];

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
        printf("ハッシュデータ配列のメモリ確保失敗\n");
        return;
    }
    // 2の冪乗サイズにすることでモジュロ演算をビットマスクに省略
    // indexを求めるとき，2の冪乗サイズで次式が同じ値を返す (code % size == code & (size-1))
    assert(CountBits(table->size) == 1);
}

void FreeHashTable(HashTable *table)
{
    free(table->data);
    table->size = 0;
}

void ResetHashTable(HashTable *table)
{
    for (size_t i = 0; i < table->size; i++)
    {
        table->data[i].own = 0;
        table->data[i].opp = 0;
        table->data[i].depth = 0;
        table->data[i].bestMove = NOMOVE_INDEX;
        table->data[i].bestMove = NOMOVE_INDEX;
        table->data[i].lower = 0.0;
        table->data[i].upper = 0.0;
    }
    ResetHashStatistics(table);
}

void ResetHashStatistics(HashTable *table)
{
    table->nbUsed = 0;
    table->nbCollide = 0;
    table->nbHit = 0;
}

// 乱数ビット列を統合してハッシュコードを取得する
inline uint64_t GetHashCode(uint64_t own, uint64_t opp)
{
    uint64_t code;

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

HashData *GetHashData(HashTable *table, uint64_t own, uint64_t opp, uint8 depth, uint64_t *hashCode)
{
    // ハッシュコード取得
    *hashCode = GetHashCode(own, opp);
    // サイズでモジュロ演算(code % size)
    uint64_t index = *hashCode & (table->size - 1);
    HashData *data = &table->data[index];

    if (data->own == own && data->opp == opp)
    {
        if (data->depth == depth)
        {
            table->nbHit++;
            return data;
        }
    }
    return NULL;
}

uint8 HashContains(HashTable *table, uint64_t own, uint64_t opp)
{
    // ハッシュコード取得
    uint64_t hashCode = GetHashCode(own, opp);
    // サイズでモジュロ演算(code % size)
    uint64_t index = hashCode & (table->size - 1);
    HashData *data = &table->data[index];

    if (data->own == own && data->opp == opp)
    {
        return 1;
    }
    return 0;
}

bool CutWithHash(HashData *hashData, float *alpha, float *beta, float *score)
{
    assert(hashData != NULL);
    if (hashData->upper <= *alpha)
    {
        *score = hashData->upper;
        return true;
    }
    if (hashData->lower >= *beta)
    {
        *score = hashData->lower;
        return true;
    }
    if (hashData->lower == hashData->upper)
    {
        *score = hashData->upper;
        return true;
    }

    *alpha = maxf(*alpha, hashData->lower);
    *beta = minf(*beta, hashData->upper);
    return false;
}

void SaveNewData(HashData *data, const uint64_t own, const uint64_t opp, const uint8 bestMove, const uint8 depth, const float alpha, const float beta, const float maxScore)
{
    if (maxScore < beta)
        data->upper = maxScore;
    else
        data->upper = Const::MAX_VALUE;

    if (maxScore > alpha)
        data->lower = maxScore;
    else
        data->lower = -Const::MAX_VALUE;

    if (maxScore > alpha || maxScore == -Const::MAX_VALUE)
        data->bestMove = bestMove;
    else
        data->bestMove = NOMOVE_INDEX;

    data->own = own;
    data->opp = opp;
    data->secondMove = NOMOVE_INDEX;
    data->depth = depth;
}

void UpdateData(HashData *data, const uint64_t own, const uint64_t opp, const uint8 bestMove, const uint8 depth, const float alpha, const float beta, const float maxScore)
{
    if (maxScore < beta && maxScore < data->upper)
        data->upper = maxScore;

    if (maxScore > alpha && maxScore > data->lower)
        data->lower = maxScore;

    if ((maxScore > alpha || maxScore == -Const::MAX_VALUE) && data->bestMove != bestMove)
    {
        data->secondMove = data->bestMove;
        data->bestMove = bestMove;
    }
}

void SaveHashData(HashTable *table, uint64_t hashCode, uint64_t own, uint64_t opp, uint8 bestMove, uint8 depth, float alpha, float beta, float maxScore)
{
    uint64_t index = hashCode & (table->size - 1);
    HashData *hashData = &table->data[index];
    if ((hashData->own | hashData->opp) == 0)
    {
        SaveNewData(hashData, own, opp, bestMove, depth, alpha, beta, maxScore);
        table->nbUsed++;
    }
    else if (hashData->own == own && hashData->opp == opp)
    {
        if (hashData->depth <= depth)
            UpdateData(hashData, own, opp, bestMove, depth, alpha, beta, maxScore);
    }
    else
    {
        table->nbCollide++;
    }
}

/* 未使用（探索関数の中で実装されている機能）
void HashOverwrite(HashTable *table, uint64_t own, uint64_t opp, uint8 depth, float lower, float upper, uint64_t hashCode)
{
    uint64_t index = hashCode & (table->size - 1);
    HashData *data = &table->data[index];

    data->own = own;
    data->opp = opp;
    data->depth = depth;
    data->lower = lower;
    data->upper = upper;
}

void HashPriorityOverwrite(HashTable *table, uint64_t own, uint64_t opp, uint8 depth, float lower, float upper, uint64_t hashCode)
{
    uint64_t index = hashCode & (table->size - 1);
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
*/