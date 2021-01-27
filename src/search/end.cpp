#include "end.h"
#include "hash.h"
#include "moves.h"
#include "../ai/eval.h"
#include "../bit_operation.h"
#include <assert.h>

inline score_t Judge(const SearchTree *tree)
{
    const uint8 nbOwn = CountBits(tree->stones->own);
    const uint8 nbOpp = 64 - tree->nbEmpty - nbOwn;
    return (score_t)((nbOwn - nbOpp) * STONE_VALUE);
}

score_t EndAlphaBeta(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed)
{
    uint8 bestMove;
    uint64_t hashCode;
    SearchFunc_t NextSearch;
    MoveList moveList;
    Move *move;
    HashData *hashData = NULL;
    score_t score, maxScore, lower;

    tree->nodeCount++;
    if (depth <= 0)
    {
        //return EvalPosTable(own, opp);
        return Judge(tree);
    }

    CreateMoveList(&moveList, tree->stones);

    // 手があるか
    if (moveList.nbMoves <= 0)
    {
        // 2連続パスなら終了
        if (passed == 1)
        {
            bestMove = NOMOVE_INDEX;
            // 勝敗判定
            return Judge(tree);
        }
        else
        {
            // 手番を入れ替えて探索続行
            SearchPassEnd(tree);
            maxScore = -EndAlphaBeta(tree, -beta, -alpha, depth, true);
            SearchPassEnd(tree);
            bestMove = PASS_INDEX;
        }
    }
    else
    {
        if (tree->useHash == 1 && depth >= tree->hashDepth)
        {
            hashData = HashTableGetData(tree->table, tree->stones, depth, &hashCode);
            if (hashData != NULL && IsHashCut(hashData, depth, &alpha, &beta, &score))
                return score;
        }
        if (depth >= tree->orderDepth)
        {
            EvaluateMoveList(tree, &moveList, tree->stones, hashData);
            //SortMoveList(&moveList);
            NextSearch = EndAlphaBeta;
        }
        else
        {
            NextSearch = EndAlphaBetaDeep;
        }
        maxScore = -Const::MAX_VALUE;
        lower = alpha;
        // 打つ手がある時, 良い手から並べ替えつつループ
        for (move = NextBestMoveWithSwap(moveList.moves); move != NULL; move = NextBestMoveWithSwap(move))
        {
            SearchUpdateEnd(tree, move);
            {
                // 子ノードを探索
                score = -NextSearch(tree, -beta, -lower, depth - 1, false);
            }
            SearchRestoreEnd(tree, move);

            if (score > maxScore)
            {
                maxScore = score;
                bestMove = move->posIdx;

                // 上限突破したら
                if (score >= beta)
                {
                    tree->nbCut++;
                    // 探索終了（カット）
                    break;
                }
                else if (maxScore > lower)
                {
                    lower = maxScore;
                }
            }
        }
    }

    if (tree->useHash == 1)
    {
        HashTableRegist(tree->table, hashCode, tree->stones, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}

score_t EndAlphaBetaDeep(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed)
{

    assert(depth <= tree->orderDepth);

    uint64_t mob, pos, flip, hashCode;
    uint8 posIdx;
    uint8 bestMove;
    HashData *hashData = NULL;
    score_t score, maxScore, lower;

    tree->nodeCount++;
    if (depth <= 0)
    {
        //return EvalPosTable(own, opp);
        //return EvalTinyDnn(tree, tree->nbEmpty);
        return Judge(tree);
    }

    if (tree->useHash == 1 && depth >= tree->hashDepth)
    {
        hashData = HashTableGetData(tree->table, tree->stones, depth, &hashCode);
        if (hashData != NULL && IsHashCut(hashData, depth, &alpha, &beta, &score))
            return score;
    }

    mob = CalcMobility(tree->stones);

    // 手があるか
    if (mob == 0)
    {
        // 2連続パスなら終了
        if (passed == 1)
        {
            bestMove = NOMOVE_INDEX;
            // 勝敗判定
            return Judge(tree);
        }
        else
        {
            // 手番を入れ替えて探索続行
            bestMove = PASS_INDEX;
            SearchPassEnd(tree);
            maxScore = -EndAlphaBetaDeep(tree, -beta, -alpha, depth, true);
            SearchPassEnd(tree);
        }
    }
    else
    {
        maxScore = -Const::MAX_VALUE;
        lower = alpha;
        // 打つ手がある時
        while (mob != 0)
        {
            // 着手位置・反転位置を取得
            pos = GetLSB(mob);
            mob ^= pos;
            posIdx = CalcPosIndex(pos);
            flip = CalcFlip(tree->stones, posIdx);

            SearchUpdateEndDeep(tree, pos, flip);
            {
                // 子ノードを探索
                score = -EndAlphaBetaDeep(tree, -beta, -lower, depth - 1, false);
            }
            SearchRestoreEndDeep(tree, pos, flip);

            if (score > maxScore)
            {
                maxScore = score;
                bestMove = posIdx;

                // 上限突破したら
                if (score >= beta)
                {
                    tree->nbCut++;
                    // 探索終了（カット）
                    break;
                }
                else if (maxScore > lower)
                {
                    lower = maxScore;
                }
            }
        }
    }

    if (tree->useHash == 1 && hashData != NULL)
    {
        HashTableRegist(tree->table, hashCode, tree->stones, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}

uint8 EndRoot(SearchTree *tree, uint8 choiceSecond)
{
    score_t score, lower, maxScore = -Const::MAX_VALUE;
    uint8 bestPos = NOMOVE_INDEX, secondPos = NOMOVE_INDEX;
    SearchFunc_t SearchFunc;
    MoveList moveList;
    Move *move;

    CreateMoveList(&moveList, tree->stones);
    assert(moveList.nbMoves > 0);
    if (tree->depth > tree->orderDepth)
    {
        EvaluateMoveList(tree, &moveList, tree->stones, NULL);
        SearchFunc = EndAlphaBeta;
    }
    else
    {
        SearchFunc = EndAlphaBetaDeep;
    }

    lower = -Const::MAX_VALUE;
    for (move = NextBestMoveWithSwap(moveList.moves); move != NULL; move = NextBestMoveWithSwap(move))
    {
        SearchUpdateEnd(tree, move);
        {
            score = -SearchFunc(tree, -Const::MAX_VALUE, -lower, tree->depth, false);
        }
        SearchRestoreEnd(tree, move);

        if (score > maxScore)
        {
            maxScore = score;
            secondPos = bestPos;
            bestPos = move->posIdx;
            if (maxScore > lower)
            {
                lower = maxScore;
            }
        }
    }

    tree->score = maxScore;

    if (choiceSecond == 1 && secondPos != NOMOVE_INDEX)
    {
        return secondPos;
    }
    return bestPos;
}