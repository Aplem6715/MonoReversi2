
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

void SearchPassMid(SearchTree *tree)
{
    UpdateEvalPass(tree->eval);
    tree->stones->opp = tree->stones->own;
    tree->stones->own = tree->stones->opp;
}

void SearchUpdateMid(SearchTree *tree, Move *move)
{
    uint64_t posBit = CalcPosBit(move->posIdx);
    UpdateEval(tree->eval, move->posIdx, move->flip);
    tree->stones->own = tree->stones->opp ^ move->flip;
    tree->stones->opp = tree->stones->own ^ move->flip ^ posBit;
}

void SearchRestoreMid(SearchTree *tree, Move *move)
{
    uint64_t posBit = CalcPosBit(move->posIdx);
    UndoEval(tree->eval, move->posIdx, move->flip);
    tree->stones->own = tree->stones->opp ^ move->flip ^ posBit;
    tree->stones->opp = tree->stones->own ^ move->flip;
}

void SearchUpdateMidDeep(SearchTree *tree, uint64_t pos)
{
    uint8 posIdx = CalcPosIndex(pos);
    uint64_t flip = CalcFlipOptimized(tree->stones->own, tree->stones->opp, posIdx);
    UpdateEval(tree->eval, posIdx, flip);
    tree->stones->own = tree->stones->opp ^ flip;
    tree->stones->opp = tree->stones->own ^ flip ^ pos;
}

void SearchRestoreMidDeep(SearchTree *tree, uint64_t pos, uint64_t rev)
{
    uint8 posIdx = CalcPosIndex(pos);
    uint64_t flip = CalcFlipOptimized(tree->stones->own, tree->stones->opp, posIdx);
    UndoEval(tree->eval, posIdx, flip);
    tree->stones->own = tree->stones->opp ^ flip ^ pos;
    tree->stones->opp = tree->stones->own ^ flip;
}

void SearchPassEnd(SearchTree *tree)
{
    tree->stones->own = tree->stones->opp;
    tree->stones->opp = tree->stones->own;
    UpdateEvalPass(tree->eval);
}

void SearchUpdateEnd(SearchTree *tree, Move *move)
{
    UpdateEval(tree->eval, move->posIdx, move->flip);
    tree->stones->own = tree->stones->opp ^ move->flip;
    tree->stones->opp = tree->stones->own ^ move->flip ^ CalcPosBit(move->posIdx);
}

void SearchRestoreEnd(SearchTree *tree, Move *move)
{
    UndoEval(tree->eval, move->posIdx, move->flip);
    tree->stones->own = tree->stones->opp ^ move->flip ^ CalcPosBit(move->posIdx);
    tree->stones->opp = tree->stones->own ^ move->flip;
}

void SearchUpdateEndDeep(SearchTree *tree, uint64_t pos)
{
    uint8 posIdx = CalcPosIndex(pos);
    uint64_t flip = CalcFlipOptimized(tree->stones->own, tree->stones->opp, posIdx);
    tree->stones->own = tree->stones->opp ^ flip;
    tree->stones->opp = tree->stones->own ^ flip ^ pos;
}

void SearchRestoreEndDeep(SearchTree *tree, uint64_t pos)
{
    uint8 posIdx = CalcPosIndex(pos);
    uint64_t flip = CalcFlipOptimized(tree->stones->own, tree->stones->opp, posIdx);
    tree->stones->own = tree->stones->opp ^ flip ^ pos;
    tree->stones->opp = tree->stones->own ^ flip;
}

uint8 Search(SearchTree *tree, uint64_t own, uint64_t opp, uint8 choiceSecond)
{
    score_t score, maxScore = -Const::MAX_VALUE;
    uint64_t mob, rev, pos;
    uint8 posIdx, bestPos = 64, secondPos = 64;
    SearchFunc_t SearchFunc;

    std::chrono::system_clock::time_point start, end;
    start = std::chrono::system_clock::now();
    tree->nodeCount = 0;
    tree->nbCut = 0;

    // 評価パターンの初期化
    ReloadEval(tree->eval, own, opp, OWN);

    if (tree->eval->nbEmpty < tree->endDepth)
    {
        tree->isEndSearch = 1;
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
        tree->isEndSearch = 0;
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
        rev = CalcFlipOptimized(own, opp, posIdx);

        UpdateEval(tree->eval, posIdx, rev);
        {
            score = -SearchFunc(tree, opp ^ rev, own ^ rev ^ pos, -Const::MAX_VALUE, Const::MAX_VALUE, tree->depth, false);
        }
        UndoEval(tree->eval, posIdx, rev);

        if (score > maxScore)
        {
            maxScore = score;
            secondPos = bestPos;
            bestPos = posIdx;
        }
    }

    end = std::chrono::system_clock::now();
    tree->usedTime = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0);
    tree->score = maxScore;

    if (choiceSecond == 1 && secondPos != 64)
    {
        return secondPos;
    }
    return bestPos;
}
