#ifndef HASH_DEFINED
#define HASH_DEFINED

#include "../stones.h"
#include "../const.h"

#define NWS_TABLE_SIZE (1 << 20)
#define PV_TABLE_SIZE (1 << 12)

// ハッシュテーブルに格納されるデータ
// 8x2 + 3 + 1*4 + 2x2 = 27[byte]
typedef struct HashData
{
    // 石情報(8x2[byte])
    uint64_t own, opp;
    // 最終使用時のハッシュ表バージョン(過去の盤面が消えていくように)
    // 1byte
    uint8 latestUsedVersion;
    // 探索コスト(depthと統合したい・・・)depthは0~64, costを0~64としても127余る
    // 25->24[byte]になればパディング含めメモリ最適化が期待できる(はず？)
    // 1byte
    uint8 cost;
    // 探索深度(1byte)
    uint8 depth;
    // 予想最善手履歴(4byte)
    uint8 bestMoves[4];
    // スコアwindow(下限値，上限値)(2x2byte)
    score_strict_t lower, upper;
} HashData;

// ハッシュテーブル
typedef struct HashTable
{
    // ハッシュデータの配列
    HashData *data;
    // サイズは2のべき乗
    size_t size;

    // バージョン: ハッシュ表が使用された探索実行数
    uint8 version;

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
void HashTableInit(HashTable *table, uint64_t size);

// ハッシュテーブルの開放
void HashTableFree(HashTable *table);

// ハッシュテーブル内のデータをリセット
void HashTableReset(HashTable *table);

void HashTableVersionUp(HashTable *table);

void HashTableResetScoreWindows(HashTable *table);

// ハッシュテーブル内の統計情報をリセット
void HashTableResetStats(HashTable *table);

//inline uint64_t GetHashCode(uint64_t own, uint64_t opp);

// ハッシュテーブル内を検索
HashData *HashTableGetData(HashTable *table, Stones *stones, uint8 depth, uint64_t *hashCode);

// ハッシュ内に含まれているか
bool IsHashTableContains(HashTable *table, Stones *stones);

// ハッシュテーブルに追加
void HashTableRegist(HashTable *table, uint64_t hashCode, Stones *stones, uint8 bestMove, uint8 cost, uint8 depth, score_t in_alpha, score_t in_beta, score_t maxScore);

// ハッシュによる枝刈りが起こるかを返し，ハッシュテーブルに登録されている情報をalpha・beta値などに適用する
bool IsHashCut(HashData *hashData, const uint8 depth, score_t *alpha, score_t *beta, score_t *score);

// ハッシュによる枝刈りが起こるかを返す
bool IsHashCutNullWindow(HashData *hashData, const uint8 depth, const score_t alpha, score_t *score);

#endif