#include "ab_node.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void InitABPool(AbPool* abPool, uint64 size, uint64 extend_size) {
    int i;
    abPool->size = size;
    abPool->extend_size = extend_size;
    abPool->pool = malloc(sizeof(AbNode*) * abPool->size);
    abPool->objects = malloc(sizeof(AbNode) * abPool->size);
    for (i = 0; i < abPool->size; i++) {
        abPool->pool[i] = &abPool->objects[i];
    }
    abPool->bottom_idx = i;
}

void DeleteABPool(AbPool* abPool) {
    free(abPool->pool);
    free(abPool->objects);
    abPool->size = 0;
    abPool->bottom_idx = 0;
}

int ExtendPool(AbPool* abPool) {
    AbNode** tmp = (AbNode**)realloc(
        abPool->pool, sizeof(AbNode*) * (abPool->size + abPool->extend_size));
    AbNode* objTmp = (AbNode*)realloc(
        abPool->objects, sizeof(AbNode) * (abPool->size + abPool->extend_size));
    if (!tmp && !objTmp) {
        printf("探索ノードプールのメモリ確保ができませんでした。\n");
        return -1;
    }

    abPool->pool = tmp;
    abPool->objects = objTmp;
    int i;
    for (i = abPool->size; i < abPool->size + abPool->extend_size; i++) {
        abPool->bottom_idx++;
        abPool->pool[abPool->bottom_idx] = &abPool->objects[i];
    }
    abPool->size = i;

    return 0;
}

AbNode* GetNewABNode(AbPool* abPool) {
    if (abPool->bottom_idx == 0) {
        if (ExtendPool(abPool) == -1) {
            return NULL;
        }
    }
    abPool->bottom_idx--;
    return abPool->pool[abPool->bottom_idx + 1];
}

void RemoveABNode(AbPool* abPool, AbNode* node) {
    abPool->bottom_idx++;
    // 上限突破チェック
    assert(abPool->bottom_idx < abPool->size);

    abPool->pool[abPool->bottom_idx] = node;
}
