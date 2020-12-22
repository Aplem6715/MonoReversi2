#include "ab_node.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void InitABPool(AbPool *abPool, uint32 size, uint32 extend_size)
{
    uint32 i;
    abPool->size = size;
    abPool->extend_size = extend_size;
    abPool->pool = (AbNode **)malloc(sizeof(AbNode *) * abPool->size);
    abPool->objects = (AbNode *)malloc(sizeof(AbNode) * abPool->size);

    if (abPool->pool == NULL)
    {
        printf("ノードプール領域の確保失敗\n");
        return;
    }
    if (abPool->objects == NULL)
    {
        printf("ノードオブジェクト領域の確保失敗\n");
        return;
    }
    for (i = 0; i < abPool->size; i++)
    {
        abPool->pool[i] = &abPool->objects[i];
    }
    abPool->bottom_idx = i - 1;
    abPool->usedNum = 0;
    abPool->stockNum = i;
}

void DeleteABPool(AbPool *abPool)
{
    free(abPool->pool);
    free(abPool->objects);
    abPool->size = 0;
    abPool->bottom_idx = 0;
}

int ExtendPool(AbPool *abPool)
{
    AbNode **tmp = (AbNode **)realloc(
        abPool->pool, sizeof(AbNode *) * (abPool->size + abPool->extend_size));
    AbNode *objTmp = (AbNode *)realloc(
        abPool->objects, sizeof(AbNode) * (abPool->size + abPool->extend_size));
    if (!tmp && !objTmp)
    {
        printf("探索ノードプールのメモリ確保ができませんでした。\n");
        return -1;
    }

    abPool->pool = tmp;
    abPool->objects = objTmp;
    uint32 i;
    for (i = abPool->size; i < abPool->size + abPool->extend_size; i++)
    {
        abPool->bottom_idx++;
        abPool->pool[abPool->bottom_idx] = &abPool->objects[i];
    }
    abPool->size = i;

    return 0;
}

AbNode *GetNewABNode(AbPool *abPool)
{
    if (abPool->bottom_idx == 0)
    {
        if (ExtendPool(abPool) == -1)
        {
            return NULL;
        }
    }

    AbNode *node = abPool->pool[abPool->bottom_idx];
    abPool->bottom_idx--;
    node->alpha = -Const::MAX_VALUE;
    node->beta = Const::MAX_VALUE;
    node->depth = 0;
    // いらないかもだけど一応
    node->opp = 0;
    node->own = 0;
    node->mob = 0;

    for (int i = 0; i < MAX_CHILD_NUM; i++)
    {
        node->childs[i] = NULL;
    }

    abPool->usedNum++;

    return node;
}

void RemoveABNode(AbPool *abPool, AbNode *node)
{
    // 子ノードの返却
    for (int i = 0; i < MAX_CHILD_NUM; i++)
    {
        if (node->childs[i] == NULL)
        {
            break;
        }
        RemoveABNode(abPool, node->childs[i]);
    }
    abPool->bottom_idx++;
    // 上限突破チェック
    assert(abPool->bottom_idx < abPool->size);

    abPool->pool[abPool->bottom_idx] = node;
    abPool->usedNum--;
}
