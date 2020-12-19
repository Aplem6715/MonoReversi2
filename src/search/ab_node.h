
#ifndef AB_NODE_DEFINED
#define AB_NODE_DEFINED

#include "const.h"

// 41[byte]
typedef struct AbNode {
    // 評価値
    float alpha, beta;
    // 評価値を計算した手番までの深さ
    unsigned char depth;
    // 自分石　相手石　着手可能位置
    uint64 own, opp, mob;
    // 下につながる子ノード
    AbNode* childs;
} AbNode;

typedef struct AbPool {
    uint64 size;
    uint64 extend_size;
    uint64 bottom_idx;
    AbNode* objects;
    AbNode** pool;
} AbPool;

void InitABPool(AbPool* abPool, uint64 size, uint64 extend_size);
void DeleteABPool(AbPool* abPool);
AbNode* GetNewABNode(AbPool* abPool);
void RemoveABNode(AbPool* abPool, AbNode* node);

#endif