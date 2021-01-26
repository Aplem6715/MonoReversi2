#ifndef HASH_DEFINED
#define HASH_DEFINED

#include "../stones.h"
#include "../const.h"

#define NOMOVE_INDEX 64
#define PASS_INDEX 65

// ハッシュテーブルに格納されるデータ
// 8x2 + 1 + 1*2 + 4x2 = 27[byte]
typedef struct HashData
{
    uint64_t own, opp;
    uint8 depth;
    uint8 bestMove, secondMove;
    //int16_t lower, upper;
    score_strict_t lower, upper;
} HashData;

// ハッシュテーブル
typedef struct HashTable
{
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

#define HASH_SEED (160510)
#define MIN_RAWHASH_BIT (8)

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
void HashTableRegist(HashTable *table, uint64_t hashCode, Stones *stones, uint8 bestMove, uint8 depth, score_t alpha, score_t beta, score_t maxScore);

// ハッシュによる枝刈りが起こるかを返し，ハッシュテーブルに登録されている情報をalpha・beta値などに適用する
bool IsHashCut(HashData *hashData, score_t *alpha, score_t *beta, score_t *score);

#endif