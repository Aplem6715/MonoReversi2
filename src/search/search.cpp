
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

    EvalInit(tree->eval);

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
    EvalDelete(tree->eval);
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

void SearchSetup(SearchTree *tree, uint64_t own, uint64_t opp)
{
    // 評価パターンの初期化
    EvalReload(tree->eval, own, opp, OWN);

    tree->nbEmpty = CountBits(~(own | opp));
    tree->nodeCount = 0;
    tree->nbCut = 0;

    tree->stones->own = own;
    tree->stones->opp = opp;
}

void SearchPassMid(SearchTree *tree)
{
    EvalUpdatePass(tree->eval);
    SwapStones(tree->stones);
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
    SwapStones(tree->stones);
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

SearchFunc_t DecideSearchFunc(SearchTree *tree)
{
    if (tree->nbEmpty < tree->endDepth)
    {
        tree->isEndSearch = 1;
        tree->depth = tree->nbEmpty;
        // オーダリングが不要な探索では探索関数を変える
        if (tree->depth > tree->orderDepth)
        {
            return EndAlphaBeta;
        }
        else
        {
            return EndAlphaBetaDeep;
        }
    }
    else
    {
        tree->isEndSearch = 0;
        tree->depth = tree->midDepth;
        // オーダリングが不要な探索では探索関数を変える
        if (tree->depth > tree->orderDepth)
        {
            return MidAlphaBeta;
        }
        else
        {
            return MidAlphaBetaDeep;
        }
    }
}

uint8 Search(SearchTree *tree, uint64_t own, uint64_t opp, uint8 choiceSecond)
{
    score_t score, maxScore = -Const::MAX_VALUE;
    uint8 bestPos = 64, secondPos = 64;
    SearchFunc_t SearchFunc;
    MoveList moveList;
    Move *move;

    std::chrono::system_clock::time_point start, end;
    start = std::chrono::system_clock::now();

    SearchSetup(tree, own, opp);

    SearchFunc = DecideSearchFunc(tree);
    CreateMoveList(&moveList, tree->stones);
    assert(moveList.nbMoves > 0);
    if (tree->depth > tree->orderDepth)
    {
        EvaluateMoveList(tree, &moveList, tree->stones, NULL);
    }
    for (move = NextBestMoveWithSwap(moveList.moves); move != NULL; move = NextBestMoveWithSwap(move))
    {
        if (tree->isEndSearch)
            SearchUpdateEnd(tree, move);
        else
            SearchUpdateMid(tree, move);
        {
            score = -SearchFunc(tree, -Const::MAX_VALUE, Const::MAX_VALUE, tree->depth, false);
        }
        if (tree->isEndSearch)
            SearchRestoreEnd(tree, move);
        else
            SearchRestoreMid(tree, move);

        if (score > maxScore)
        {
            maxScore = score;
            secondPos = bestPos;
            bestPos = move->posIdx;
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
