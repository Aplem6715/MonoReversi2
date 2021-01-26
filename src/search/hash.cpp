#include "hash.h"
#include "../bit_operation.h"
#include <random>
#include <stdlib.h>
#include <assert.h>

#define HASH_TABLE_SIZE (1 << 24)
#define RETRY_HASH(h) ((h) ^ 1)

// RawHash[8行x2色][列内8石のパターン]
uint64_t RawHash[8 * 2][1 << 8];

void HashInit()
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

void HashTableInit(HashTable *table)
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

void HashTableFree(HashTable *table)
{
    free(table->data);
    table->size = 0;
}

void HashTableReset(HashTable *table)
{
    for (size_t i = 0; i < table->size; i++)
    {
        table->data[i].own = 0;
        table->data[i].opp = 0;
        table->data[i].depth = 0;
        table->data[i].bestMove = NOMOVE_INDEX;
        table->data[i].bestMove = NOMOVE_INDEX;
        table->data[i].lower = 0;
        table->data[i].upper = 0;
    }
    HashTableResetStats(table);
}

void HashTableResetStats(HashTable *table)
{
    table->nbUsed = 0;
    table->nb2ndUsed = 0;
    table->nbCollide = 0;
    table->nbHit = 0;
    table->nb2ndHit = 0;
}

// 乱数ビット列を統合してハッシュコードを取得する
inline uint64_t GetHashCode(Stones *stones)
{
    uint64_t code;

    // own: 64bit -> 8x8bit
    const uint8 *cursor = (uint8 *)(&stones->own);
    code = RawHash[0][cursor[0]];
    code ^= RawHash[1][cursor[1]];
    code ^= RawHash[2][cursor[2]];
    code ^= RawHash[3][cursor[3]];
    code ^= RawHash[4][cursor[4]];
    code ^= RawHash[5][cursor[5]];
    code ^= RawHash[6][cursor[6]];
    code ^= RawHash[7][cursor[7]];

    // opp: 64bit -> 8x8bit
    cursor = (uint8 *)(&stones->opp);
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

HashData *HashTableGetData(HashTable *table, Stones *stones, uint8 depth, uint64_t *hashCode)
{
    // ハッシュコード取得
    *hashCode = GetHashCode(stones);
    // サイズでモジュロ演算(code % size)
    uint64_t index = *hashCode & (table->size - 1);
    HashData *data = &table->data[index];

    // データの衝突チェック（同じ盤面かどうかで確認）
    if (data->own == stones->own && data->opp == stones->opp)
    {
        if (data->depth == depth)
        {
            table->nbHit++;
            return data;
        }
    }

    // インデックスを再計算して再取得を試す
    data = &table->data[RETRY_HASH(index)];
    if (data->own == stones->own && data->opp == stones->opp)
    {
        if (data->depth == depth)
        {
            table->nb2ndHit++;
            return data;
        }
    }

    return NULL;
}

uint8 IsHashTableContains(HashTable *table, Stones *stones)
{
    // ハッシュコード取得
    uint64_t hashCode = GetHashCode(stones);
    // サイズでモジュロ演算(code % size)
    uint64_t index = hashCode & (table->size - 1);

    HashData *data = &table->data[index];
    if (data->own == stones->own && data->opp == stones->opp)
    {
        return 1;
    }

    data = &table->data[RETRY_HASH(index)];
    if (data->own == stones->own && data->opp == stones->opp)
    {
        return 1;
    }
    return 0;
}

bool IsHashCut(HashData *hashData, score_t *alpha, score_t *beta, score_t *score)
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

    *alpha = MAX(*alpha, hashData->lower);
    *beta = MIN(*beta, hashData->upper);
    return false;
}

void HashDataSaveNew(HashData *data, const Stones *stones, const uint8 bestMove, const uint8 depth, const score_t alpha, const score_t beta, const score_t maxScore)
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

    data->own = stones->own;
    data->opp = stones->opp;
    data->secondMove = NOMOVE_INDEX;
    data->depth = depth;
}

/**
 * @brief ハッシュデータ内の更新が必要な値を更新
 * 
 * @param data 更新するデータ
 * @param bestMove 見つかった最善手
 * @param depth 探索深度
 * @param alpha アルファ値
 * @param beta ベータ値
 * @param maxScore 最善手のスコア
 */
void HashDataUpdate(HashData *data, const uint8 bestMove, const uint8 depth, const score_t alpha, const score_t beta, const score_t maxScore)
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

/**
 * @brief 盤面は同じだが，探索深度が更に深い場合，深い探索データに合わせるよう最善手以外を初期化
 * 
 * @param data 更新するデータ
 * @param bestMove 見つかった最善手
 * @param depth 探索深度
 * @param alpha アルファ値
 * @param beta ベータ値
 * @param maxScore 最善手のスコア
 */
void HashDataLevelUP(HashData *data, const uint8 bestMove, const uint8 depth, const score_t alpha, const score_t beta, const score_t maxScore)
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
    {
        data->secondMove = data->bestMove;
        data->bestMove = bestMove;
    }
    else
    {
        data->bestMove = NOMOVE_INDEX;
    }

    data->depth = depth;
}

uint8 HashDataPriorityOverwrite(HashData *data, const Stones *stones, const uint8 bestMove, const uint8 depth, const score_t alpha, const score_t beta, const score_t maxScore)
{
    if (depth > data->depth)
    {
        HashDataSaveNew(data, stones, bestMove, depth, alpha, beta, maxScore);
        return 1;
    }
    return 0;
}

void HashTableRegist(HashTable *table, uint64_t hashCode, Stones *stones, uint8 bestMove, uint8 depth, score_t alpha, score_t beta, score_t maxScore)
{
    uint64_t index = hashCode & (table->size - 1);
    HashData *hashData = &table->data[index];
    if ((hashData->own | hashData->opp) == 0)
    {
        HashDataSaveNew(hashData, stones, bestMove, depth, alpha, beta, maxScore);
        table->nbUsed++;
    }
    else if (hashData->own == stones->own && hashData->opp == stones->opp)
    {
        if (hashData->depth <= depth)
            HashDataUpdate(hashData, bestMove, depth, alpha, beta, maxScore);
    }
    else
    {
        // 見つからなかったらインデックスを再計算
        hashData = &table->data[RETRY_HASH(index)];
        if ((hashData->own | hashData->opp) == 0)
        {
            HashDataSaveNew(hashData, stones, bestMove, depth, alpha, beta, maxScore);
            table->nb2ndUsed++;
        }
        else if (hashData->own == stones->own && hashData->opp == stones->opp)
        {
            if (hashData->depth <= depth)
                HashDataUpdate(hashData, bestMove, depth, alpha, beta, maxScore);
        }
        else
        {
            // 優先度が高いデータだったら上書き
            hashData = &table->data[index];
            if (!HashDataPriorityOverwrite(hashData, stones, bestMove, depth, alpha, beta, maxScore))
            {
                // 上書きされなかったら，次のインデックスで上書きを試す
                hashData = &table->data[RETRY_HASH(index)];
                if (!HashDataPriorityOverwrite(hashData, stones, bestMove, depth, alpha, beta, maxScore))
                {
                    // それでもだめなら衝突判定
                    table->nbCollide++;
                }
            }
        }
    }
}
