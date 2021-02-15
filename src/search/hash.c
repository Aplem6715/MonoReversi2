/**
 * @file hash.c
 * @author Daichi Sato
 * @brief TranspositionTable(置換表)の実装
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 * ハッシュを用いた高速検索可能な置換表
 * 一度探索したことがある盤面の過去の情報を記録しておき，
 * 有効に再利用することができる。
 * 
 * ハッシュ関数：
 * 64x2bit盤面を1byteごとに分割し，
 * それぞれに対して乱数を振り分け（起動時に各バイト値(0-255)に乱数振り分けをしておく），
 * 取得された16個の乱数をXORで統合することでハッシュ値を生成。
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "hash.h"
#include "random_util.h"
#include "../bit_operation.h"
#include "../const.h"

// データ位置再探索（偶奇反転）
#define RETRY_HASH(h) ((h) ^ 1)

#define HASH_SEED (160510)
#define RAWHASH_MIN_TRUE_BITS (8)

// RawHash[8行x2色][列内8石のパターン]
uint64_t RawHash[8 * 2][1 << 8];

// 空ハッシュデータ
static const HashData EMPTY_HASH_DATA = {
    0, 0,                       // stones
    0,                          // depth
    NOMOVE_INDEX, NOMOVE_INDEX, // moves
    -MAX_VALUE, MAX_VALUE       // scores
};

/**
 * @brief ハッシュ関数の乱数初期化
 */
void HashInit()
{
    for (int row = 0; row < 8 * 2; row++)
    {
        for (int b = 0; b < (1 << 8); b++)
        {
            do
            {
                RawHash[row][b] = rand64();
            } while (CountBits(RawHash[row][b]) < RAWHASH_MIN_TRUE_BITS);
        }
    }
}

/**
 * @brief ハッシュ表のメモリ確保
 * 
 * @param table 初期化するハッシュ表
 * @param size ハッシュ表の要素数
 */
void HashTableInit(HashTable *table, uint64_t size)
{
    table->size = size;
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

/**
 * @brief ハッシュ表の解放
 * 
 * @param table 開放するハッシュ表
 */
void HashTableFree(HashTable *table)
{
    free(table->data);
    table->size = 0;
}

/**
 * @brief ハッシュ表のリセット
 * 
 * @param table リセットするハッシュ表
 */
void HashTableReset(HashTable *table)
{
    for (size_t i = 0; i < table->size; i++)
    {
        table->data[i] = EMPTY_HASH_DATA;
    }
    HashTableResetStats(table);
}

/**
 * @brief ハッシュ表の利用状況などの統計をリセット
 * 
 * @param table 統計をリセットするハッシュ表
 */
void HashTableResetStats(HashTable *table)
{
    table->nbUsed = 0;
    table->nb2ndUsed = 0;
    table->nbCollide = 0;
    table->nbHit = 0;
    table->nb2ndHit = 0;
}

/**
 * @brief 乱数ビット列を統合してハッシュコードを取得する
 * 
 * @param stones 盤面の石情報
 * @return uint64_t ハッシュコード
 */
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

/**
 * @brief ハッシュ表内から，指定盤面のハッシュデータを取得
 * 
 * @param table ハッシュ表
 * @param stones 盤面情報
 * @param depth 探索深度
 * @param hashCode ハッシュコード
 * @return HashData* 取得したハッシュデータ
 */
HashData *HashTableGetData(HashTable *table, Stones *stones, uint8 depth, uint64_t *hashCode)
{
    // ハッシュコード取得
    *hashCode = GetHashCode(stones);
    // サイズでモジュロ演算(code % size)
    uint64_t index = *hashCode & (table->size - 1);
    HashData *data = &table->data[index];
    HashData *secondData;

    // データの衝突チェック（同じ盤面かどうかで確認）
    if (data->own == stones->own && data->opp == stones->opp)
    {
        table->nbHit++;
        return data;
    }

    // インデックスを再計算して再取得を試す
    secondData = &table->data[RETRY_HASH(index)];
    if (secondData->own == stones->own && secondData->opp == stones->opp)
    {
        table->nb2ndHit++;
        return secondData;
    }

    return NULL;
}

/**
 * @brief 指定盤面のデータを含んでいるかどうか
 * 
 * @param table ハッシュ表
 * @param stones 盤面情報
 * @return uint8 True/False
 */
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

/**
 * @brief ハッシュによる枝刈り・探索範囲の縮小
 * 
 * @param hashData ハッシュデータ
 * @param depth 探索深度
 * @param alpha アルファ値への参照
 * @param beta ベータ値への参照
 * @param score 探索スコアへの参照
 * @return bool 枝刈りが起こるかどうか
 */
bool IsHashCut(HashData *hashData, const uint8 depth, score_t *alpha, score_t *beta, score_t *score)
{
    assert(hashData != NULL);
    assert(hashData->lower <= hashData->upper);
    if (hashData->depth >= depth)
    {
        if (hashData->lower == hashData->upper)
        {
            *score = hashData->upper;
            return true;
        }
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

        *alpha = MAX(*alpha, hashData->lower);
        *beta = MIN(*beta, hashData->upper);
    }
    return false;
}

/**
 * @brief Null Window Search中のハッシュによる枝刈り
 * 
 * NWS中はアルファ値・ベータ値の変更は行わず，カットだけ実行。
 * 
 * @param hashData ハッシュデータ
 * @param depth 探索深度
 * @param alpha アルファ値
 * @param score 探索スコアへの参照
 * @return bool 枝刈りが起こるかどうか
 */
bool IsHashCutNullWindow(HashData *hashData, const uint8 depth, const score_t alpha, score_t *score)
{
    assert(hashData != NULL);
    assert(hashData->lower <= hashData->upper);

    if (hashData->depth >= depth)
    {
        if (alpha < hashData->lower)
        {
            *score = hashData->lower;
            return true;
        }
        if (alpha >= hashData->upper)
        {
            *score = hashData->upper;
            return true;
        }
    }
    return false;
}

/**
 * @brief ハッシュデータを新しいデータで上書きする
 * 
 * @param data 上書きするデータ
 * @param stones 盤面石情報
 * @param bestMove 予測される最善手
 * @param depth 探索深度
 * @param alpha アルファ値
 * @param beta ベータ値
 * @param maxScore 最大探索スコア
 */
void HashDataSaveNew(HashData *data, const Stones *stones, const uint8 bestMove, const uint8 depth, const score_t alpha, const score_t beta, const score_t maxScore)
{
    if (maxScore < beta)
        data->upper = maxScore;
    else
        data->upper = MAX_VALUE;

    if (maxScore > alpha)
        data->lower = maxScore;
    else
        data->lower = -MAX_VALUE;

    if (maxScore > alpha || maxScore == -MAX_VALUE)
        data->bestMove = bestMove;
    else
        data->bestMove = NOMOVE_INDEX;

    data->own = stones->own;
    data->opp = stones->opp;
    data->secondMove = NOMOVE_INDEX;
    data->depth = depth;
    assert(data->lower <= data->upper);
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
void HashDataUpdate(HashData *data, const uint8 bestMove, const score_t alpha, const score_t beta, const score_t maxScore)
{
    if (maxScore < beta && maxScore < data->upper)
        data->upper = maxScore;

    if (maxScore > alpha && maxScore > data->lower)
        data->lower = maxScore;

    if ((maxScore > alpha || maxScore == -MAX_VALUE) && data->bestMove != bestMove)
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
        data->upper = MAX_VALUE;

    if (maxScore > alpha)
        data->lower = maxScore;
    else
        data->lower = -MAX_VALUE;

    if (maxScore > alpha || maxScore == -MAX_VALUE)
    {
        data->secondMove = data->bestMove;
        data->bestMove = bestMove;
    }
    else
    {
        data->bestMove = NOMOVE_INDEX;
    }

    data->depth = depth;
    assert(data->lower <= data->upper);
}

/**
 * @brief 優先度をつけてハッシュデータを上書きする
 * 
 * @param data ハッシュデータ
 * @param stones 盤面の状態
 * @param bestMove 発見された最善手
 * @param depth データ取得時の探索深度
 * @param alpha アルファ値
 * @param beta ベータ値
 * @param maxScore 最善手のスコア
 * @return uint8 上書きされたか
 */
uint8 HashDataPriorityOverwrite(HashData *data, const Stones *stones, const uint8 bestMove, const uint8 depth, const score_t alpha, const score_t beta, const score_t maxScore)
{
    if (depth > data->depth)
    {
        HashDataSaveNew(data, stones, bestMove, depth, alpha, beta, maxScore);
        return 1;
    }
    return 0;
}

/**
 * @brief ハッシュ表に探索情報を記録する
 * 
 * 探索情報によって優先度付けを行い，衝突した際は優先度の高い方を記録する。
 * 
 * @param table ハッシュ表
 * @param hashCode ハッシュコード
 * @param stones 盤面石情報
 * @param bestMove 予測される最善手
 * @param depth 探索深度
 * @param in_alpha アルファ値
 * @param in_beta ベータ値
 * @param maxScore 最大スコア
 */
void HashTableRegist(HashTable *table, uint64_t hashCode, Stones *stones, uint8 bestMove, uint8 depth, score_t in_alpha, score_t in_beta, score_t maxScore)
{
    uint64_t index = hashCode & (table->size - 1);
    HashData *hashData = &table->data[index];
    HashData *secondData;

    if ((hashData->own | hashData->opp) == 0)
    {
        HashDataSaveNew(hashData, stones, bestMove, depth, in_alpha, in_beta, maxScore);
        table->nbUsed++;
    }
    else if (hashData->own == stones->own && hashData->opp == stones->opp)
    {
        if (depth == hashData->depth)
        {
            HashDataUpdate(hashData, bestMove, in_alpha, in_beta, maxScore);
            if (hashData->lower > hashData->upper)
            {
                HashDataSaveNew(hashData, stones, bestMove, depth, in_alpha, in_beta, maxScore);
            }
        }
        else
        {
            HashDataLevelUP(hashData, bestMove, depth, in_alpha, in_beta, maxScore);
        }
    }
    else
    {
        // 見つからなかったらインデックスを再計算
        secondData = &table->data[RETRY_HASH(index)];
        if ((secondData->own | secondData->opp) == 0)
        {
            HashDataSaveNew(secondData, stones, bestMove, depth, in_alpha, in_beta, maxScore);
            table->nb2ndUsed++;
        }
        else if (secondData->own == stones->own && secondData->opp == stones->opp)
        {
            if (depth == secondData->depth)
            {
                HashDataUpdate(secondData, bestMove, in_alpha, in_beta, maxScore);
                if (secondData->lower > secondData->upper)
                {
                    HashDataSaveNew(secondData, stones, bestMove, depth, in_alpha, in_beta, maxScore);
                }
            }
            else
            {
                HashDataLevelUP(secondData, bestMove, depth, in_alpha, in_beta, maxScore);
            }
        }
        else
        {
            // 優先度が高いデータだったら上書き
            if (!HashDataPriorityOverwrite(hashData, stones, bestMove, depth, in_alpha, in_beta, maxScore))
            {
                // 上書きされなかったら，次のインデックスで上書きを試す
                if (!HashDataPriorityOverwrite(secondData, stones, bestMove, depth, in_alpha, in_beta, maxScore))
                {
                    // それでもだめなら衝突判定
                    table->nbCollide++;
                }
            }
        }
    }
}
