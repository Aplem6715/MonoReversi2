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
    uint8 nbTurn;
    float lower, upper;
} HashData;

#define HASH_SEED (160510)
#define MIN_RAWHASH_BIT (16)

void InitHash();
uint64 GetHashCode(uint64 own, uint64 opp);

#endif