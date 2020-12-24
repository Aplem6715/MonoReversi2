#ifndef HASH_DEFINED
#define HASH_DEFINED

typedef unsigned long long uint64;
typedef unsigned char uint8;

enum HashHitState
{
    HASH_HIT,
    HASH_EMPTY,
    HASH_DEFFERENT,
    HASH_DEEPER,
    HASH_SHALLOWER
};

// 8x2 + 1 + 4x2 = 25[byte]
typedef struct HashData
{
    uint64 own, opp;
    uint8 depth;
    float lower, upper;
} HashData;

typedef struct HashTable
{
    HashData *data;
    // 必ず2のべき乗
    uint64 size;

    //　↓以降↓ 測定用
    uint64 nbUsed;
    uint64 nbHit;
    uint64 nbCollide;
} HashTable;

#define HASH_SEED (160510)
#define MIN_RAWHASH_BIT (16)

void InitHash();
void InitHashTable(HashTable *table);
void FreeHashTable(HashTable *table);
//inline uint64 GetHashCode(uint64 own, uint64 opp);

HashData *GetHashData(HashTable *table, uint64 own, uint64 opp, uint8 depth, uint64 *hashCode, HashHitState *hitState);
void HashOverwrite(HashTable *table, uint64 own, uint64 opp, uint8 depth, float lower, float upper, uint64 hashCode);
void HashPriorityOverwrite(HashTable *table, uint64 own, uint64 opp, uint8 depth, float lower, float upper, uint64 hashCode);

#endif