
#include "search.h"
#include "mid.h"
#include "end.h"
#include "../ai/nnet.h"
#include "../bit_operation.h"
#include <chrono>
#include <assert.h>

static const uint8 FIRST_MOVES_INDEX[] = {19, 26, 37, 44};

void InitTree(SearchTree *tree, unsigned char midDepth, unsigned char endDepth, unsigned char midPvsDepth, unsigned char endPvsDepth, unsigned char useHash, unsigned char useMPC)
{
    tree->midDepth = midDepth;
    tree->endDepth = endDepth;
    tree->midPvsDepth = midPvsDepth;
    tree->endPvsDepth = endPvsDepth;
    tree->useHash = useHash;
    tree->useMPC = useMPC;

    EvalInit(tree->eval);

    if (useHash)
    {
        tree->table = (HashTable *)malloc(sizeof(HashTable));
        if (tree->table == NULL)
        {
            printf("ハッシュテーブルのメモリ確保失敗\n");
            return;
        }

        HashTableInit(tree->table);
    }
}

void DeleteTree(SearchTree *tree)
{
    EvalDelete(tree->eval);
    if (tree->useHash)
    {
        HashTableFree(tree->table);
        free(tree->table);
    }
}

void ConfigTree(SearchTree *tree, unsigned char midDepth, unsigned char endDepth)
{
    tree->midDepth = midDepth;
    tree->endDepth = endDepth;
}

void ResetTree(SearchTree *tree)
{
    if (tree->useHash)
        HashTableReset(tree->table);
}

void SearchSetup(SearchTree *tree, uint64_t own, uint64_t opp)
{
    //ResetTree(tree);
    // 評価パターンの初期化
    EvalReload(tree->eval, own, opp, OWN);

    tree->nbEmpty = CountBits(~(own | opp));
    tree->nodeCount = 0;
    tree->nbCut = 0;
    tree->nbMpcNested = 0;

    tree->stones->own = own;
    tree->stones->opp = opp;
}

void SearchPassMid(SearchTree *tree)
{
    EvalUpdatePass(tree->eval);
    StonesSwap(tree->stones);
}

void SearchUpdateMid(SearchTree *tree, Move *move)
{
    uint64_t posBit = CalcPosBit(move->posIdx);
    EvalUpdate(tree->eval, move->posIdx, move->flip);
    StonesUpdate(tree->stones, posBit, move->flip);
    tree->nbEmpty--;
}

void SearchRestoreMid(SearchTree *tree, Move *move)
{
    uint64_t posBit = CalcPosBit(move->posIdx);
    EvalUndo(tree->eval, move->posIdx, move->flip);
    StonesRestore(tree->stones, posBit, move->flip);
    tree->nbEmpty++;
}

void SearchUpdateMidDeep(SearchTree *tree, uint64_t pos, uint64_t flip)
{
    uint8 posIdx = CalcPosIndex(pos);
    EvalUpdate(tree->eval, posIdx, flip);
    StonesUpdate(tree->stones, pos, flip);
    tree->nbEmpty--;
}

void SearchRestoreMidDeep(SearchTree *tree, uint64_t pos, uint64_t flip)
{
    uint8 posIdx = CalcPosIndex(pos);
    EvalUndo(tree->eval, posIdx, flip);
    StonesRestore(tree->stones, pos, flip);
    tree->nbEmpty++;
}

void SearchPassEnd(SearchTree *tree)
{
    StonesSwap(tree->stones);
    EvalUpdatePass(tree->eval);
}

void SearchUpdateEnd(SearchTree *tree, Move *move)
{
    EvalUpdate(tree->eval, move->posIdx, move->flip);
    StonesUpdate(tree->stones, CalcPosBit(move->posIdx), move->flip);
    tree->nbEmpty--;
}

void SearchRestoreEnd(SearchTree *tree, Move *move)
{
    EvalUndo(tree->eval, move->posIdx, move->flip);
    StonesRestore(tree->stones, CalcPosBit(move->posIdx), move->flip);
    tree->nbEmpty++;
}

void SearchUpdateEndDeep(SearchTree *tree, uint64_t pos, uint64_t flip)
{
    uint8 posIdx = CalcPosIndex(pos);
    StonesUpdate(tree->stones, pos, flip);
    tree->nbEmpty--;
}

void SearchRestoreEndDeep(SearchTree *tree, uint64_t pos, uint64_t flip)
{
    uint8 posIdx = CalcPosIndex(pos);
    StonesRestore(tree->stones, pos, flip);
    tree->nbEmpty++;
}

uint8 Search(SearchTree *tree, uint64_t own, uint64_t opp, uint8 choiceSecond)
{
    uint8 pos = NOMOVE_INDEX;
    uint8 nbEmpty = CountBits(~(own | opp));

    std::chrono::system_clock::time_point start, end;
    start = std::chrono::system_clock::now();

    SearchSetup(tree, own, opp);

    if (tree->nbEmpty == 60)
    {
        pos = FIRST_MOVES_INDEX[rand() % 4];
    }
    else if (nbEmpty <= tree->endDepth)
    {
        tree->isEndSearch = 1;
        tree->depth = nbEmpty;
        tree->pvsDepth = tree->endPvsDepth;
        tree->orderDepth = tree->pvsDepth;
        tree->hashDepth = tree->pvsDepth - 1;
        pos = EndRoot(tree, choiceSecond);
    }
    else
    {
        tree->isEndSearch = 0;
        tree->depth = tree->midDepth;
        tree->pvsDepth = tree->midPvsDepth;
        tree->orderDepth = tree->pvsDepth;
        tree->hashDepth = tree->pvsDepth - 1;
        pos = MidRoot(tree, choiceSecond);
    }

    end = std::chrono::system_clock::now();
    tree->usedTime = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0);

    sprintf_s(tree->msg, "思考時間：%.2f[s]  探索ノード数：%zu[Node]  探索速度：%.1f[Node/s]  推定CPUスコア：%.1f",
              tree->usedTime,
              tree->nodeCount,
              tree->nodeCount / tree->usedTime,
              tree->score / (float)(STONE_VALUE));

    assert(tree->nbMpcNested == 0);
    return pos;
}
