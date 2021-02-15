#ifndef HASH_DEFINED
#define HASH_DEFINED

#include "../stones.h"
#include "../const.h"

// ハッシュテーブルに格納されるデータ
// 8x2 + 1 + 1*2 + 2x2 = 23[byte]
typedef struct HashData
{
    // 石情報
    uint64_t own, opp;
    // 探索深度
    uint8 depth;
    // 最善手，更新前の最善手
    uint8 bestMove, secondMove;
    // スコアwindow(下限値，上限値)
    score_strict_t lower, upper;
} HashData;

// ハッシュテーブル
typedef struct HashTable
{
    // ハッシュデータの配列
    HashData *data;
    // サイズは2のべき乗
    size_t size;

    /* 計測用 */
    uint64_t nbUsed;
    uint64_t nb2ndUsed;
    uint64_t nbHit;
    uint64_t nb2ndHit;
    uint64_t nbCollide;
} HashTable;

// ハッシュキー生成用の乱数ビット列を初期化
void HashInit();

// ハッシュテーブルの初期化
void HashTableInit(HashTable *table);

// ハッシュテーブルの開放
void HashTableFree(HashTable *table);

// ハッシュテーブル内のデータをリセット
void HashTableReset(HashTable *table);

// ハッシュテーブル内の統計情報をリセット
void HashTableResetStats(HashTable *table);

//inline uint64_t GetHashCode(uint64_t own, uint64_t opp);

// ハッシュテーブル内を検索
HashData *HashTableGetData(HashTable *table, Stones *stones, uint8 depth, uint64_t *hashCode);

// ハッシュ内に含まれているか
uint8 IsHashTableContains(HashTable *table, Stones *stones);

// ハッシュテーブルに追加
void HashTableRegist(HashTable *table, uint64_t hashCode, Stones *stones, uint8 bestMove, uint8 depth, score_t in_alpha, score_t in_beta, score_t maxScore);

// ハッシュによる枝刈りが起こるかを返し，ハッシュテーブルに登録されている情報をalpha・beta値などに適用する
bool IsHashCut(HashData *hashData, const uint8 depth, score_t *alpha, score_t *beta, score_t *score);

// ハッシュによる枝刈りが起こるかを返す
bool IsHashCutNullWindow(HashData *hashData, const uint8 depth, const score_t alpha, score_t *score);

#endif