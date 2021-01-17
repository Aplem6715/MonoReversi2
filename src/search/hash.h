#ifndef HASH_DEFINED
#define HASH_DEFINED

typedef unsigned long long uint64;
typedef unsigned char uint8;

#define NOMOVE_INDEX 64
#define PASS_INDEX 65

// ハッシュデータ検索の際の検索結果
enum HashHitState
{
    HASH_HIT,       // ヒット
    HASH_EMPTY,     // 未使用領域がヒット
    HASH_DEFFERENT, // 他盤面と衝突
    HASH_DEEPER,    // より深い探索でのデータを発見
    HASH_SHALLOWER  // より浅い探索でのデータを発見
};

// ハッシュテーブルに格納されるデータ
// 8x2 + 1 + 1*2 + 4x2 = 27[byte]
typedef struct HashData
{
    uint64 own, opp;
    uint8 depth;
    uint8 bestMove, secondMove;
    float lower, upper;
} HashData;

// ハッシュテーブル
typedef struct HashTable
{
    HashData *data;
    // 必ず2のべき乗
    size_t size;

    //　↓以降↓ 測定用
    uint64 nbUsed;
    uint64 nbHit;
    uint64 nbCollide;
} HashTable;

#define HASH_SEED (160510)
#define MIN_RAWHASH_BIT (8)

// ハッシュキー生成用の乱数ビット列を初期化
void InitHash();
// ハッシュテーブルの初期化
void InitHashTable(HashTable *table);
// ハッシュテーブルの開放
void FreeHashTable(HashTable *table);
// ハッシュテーブル内のデータをリセット
void ResetHashTable(HashTable *table);
// ハッシュテーブル内の統計情報をリセット
void ResetHashStatistics(HashTable *table);
//inline uint64 GetHashCode(uint64 own, uint64 opp);

// ハッシュテーブル内を検索
HashData *GetHashData(HashTable *table, uint64 own, uint64 opp, uint8 depth, uint64 *hashCode);
// ハッシュ内に含まれているか
uint8 HashContains(HashTable *table, uint64 own, uint64 opp);

bool CutWithHash(HashData *hashData, float *alpha, float *beta, float *score);

void SaveHashData(HashTable *table, uint64 hashCode, uint64 own, uint64 opp, uint8 bestMove, uint8 depth, float alpha, float beta, float maxScore);
/* 未使用（探索関数の中で実装されている機能）
void HashOverwrite(HashTable *table, uint64 own, uint64 opp, uint8 depth, float lower, float upper, uint64 hashCode);
void HashPriorityOverwrite(HashTable *table, uint64 own, uint64 opp, uint8 depth, float lower, float upper, uint64 hashCode);
*/

#endif