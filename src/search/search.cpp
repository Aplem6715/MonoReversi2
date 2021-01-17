
#include "hash.h"
#include "moves.h"
#include "search.h"
#include "mid.h"
#include "end.h"
#include "../ai/nnet.h"
#include "../bit_operation.h"
#include <chrono>
#include <assert.h>

void InitTree(SearchTree *tree, unsigned char midDepth, unsigned char endDepth, unsigned char orderDepth, unsigned char useHash, unsigned char hashDepth)
{
    tree->midDepth = midDepth;
    tree->endDepth = endDepth;
    tree->orderDepth = orderDepth;
    tree->useHash = useHash;
    tree->hashDepth = hashDepth;

    InitEval(tree->eval);

    if (useHash)
    {
        tree->table = (HashTable *)malloc(sizeof(HashTable));
        if (tree->table == NULL)
        {
            printf("ハッシュテーブルのメモリ確保失敗\n");
            return;
        }

        InitHashTable(tree->table);
    }
}

void DeleteTree(SearchTree *tree)
{
    DeleteEval(tree->eval);
    if (tree->useHash)
    {
        FreeHashTable(tree->table);
        free(tree->table);
    }
#ifdef USE_NN
    free(tree->eval->net);
#elif USE_REGRESSION
    free(tree->eval->regr);
#endif
}

void ConfigTree(SearchTree *tree, unsigned char midDepth, unsigned char endDepth)
{
    tree->midDepth = midDepth;
    tree->endDepth = endDepth;
}

void ResetTree(SearchTree *tree)
{
    if (tree->useHash)
        ResetHashTable(tree->table);
}

uint64 Search(SearchTree *tree, uint64 own, uint64 opp, uint8 choiceSecond)
{
    float score, maxScore = -Const::MAX_VALUE;
    uint64 bestPos = 3, pos = 3, secondPos = 3, mob, rev;
    uint8 posIdx;
    SearchFunc_t SearchFunc;

    std::chrono::system_clock::time_point start, end;
    start = std::chrono::system_clock::now();
    tree->nodeCount = 0;

    // 評価パターンの初期化
    ReloadEval(tree->eval, own, opp, OWN);

    if (tree->eval->nbEmpty < tree->endDepth)
    {
        tree->depth = tree->eval->nbEmpty;
        // オーダリングが不要な探索では探索関数を変える
        if (tree->depth > tree->orderDepth)
        {
            SearchFunc = EndAlphaBeta;
        }
        else
        {
            SearchFunc = EndAlphaBetaDeep;
        }
    }
    else
    {
        tree->depth = tree->midDepth;
        // オーダリングが不要な探索では探索関数を変える
        if (tree->depth > tree->orderDepth)
        {
            SearchFunc = MidAlphaBeta;
        }
        else
        {
            SearchFunc = MidAlphaBetaDeep;
        }
    }

    mob = CalcMobility(own, opp);
    assert(mob != 0);
    while (mob != 0)
    {
        pos = GetLSB(mob);
        posIdx = CalcPosIndex(pos);
        mob ^= pos;
        rev = CalcFlip(own, opp, pos);

        UpdateEval(tree->eval, posIdx, rev);
        {
            score = -SearchFunc(tree, opp ^ rev, own ^ rev ^ pos, -Const::MAX_VALUE, Const::MAX_VALUE, tree->depth, false);
        }
        UndoEval(tree->eval, posIdx, rev);

        if (score > maxScore)
        {
            maxScore = score;
            secondPos = bestPos;
            bestPos = pos;
        }
    }

    end = std::chrono::system_clock::now();
    tree->usedTime = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0);
    tree->score = maxScore;

    if (choiceSecond == 1 && secondPos != 3)
    {
        return secondPos;
    }
    return bestPos;
}
