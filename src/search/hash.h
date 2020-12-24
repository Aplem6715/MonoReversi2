#ifndef HASH_DEFINED
#define HASH_DEFINED

typedef unsigned long long uint64;
typedef unsigned char uint8;

typedef struct Stone128
{
    uint64 own, opp;
} Stone128;

typedef struct HashData
{
    Stone128 stones;
    uint8 depth;
    float lower, upper;
} HashData;

typedef struct HashTable
{
    HashData *data;
    uint64 size;

    //　↓以降↓ 測定用
    uint64 nbUsed;
    uint64 nbHit;
    uint64 nbCollide;
} HashTable;

#define HASH_SEED (160510)
#define MIN_RAWHASH_BIT (16)

void InitHash();
void InitHashTable(HashTable &table);
void FreeHashTable(HashTable &table);
uint64 GetHashCode(uint64 own, uint64 opp);

#endif