#include "mid.h"
#include "hash.h"
#include "moves.h"
#include "../ai/eval.h"
#include "../bit_operation.h"
#include <assert.h>

inline score_t WinJudge(const Stones *stones)
{
    uint8 ownCnt = CountBits(stones->own);
    uint8 oppCnt = CountBits(stones->opp);
    if (ownCnt > oppCnt)
    {
        return SCORE_MAX;
    }
    else if (ownCnt < oppCnt)
    {
        return SCORE_MIN;
    }
    else
    {
        return 0;
    }
}

score_t MidAlphaBeta(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed)
{
    SearchFunc_t NextSearch;
    HashData *hashData = NULL;
    MoveList moveList;
    Move *move;
    uint64_t hashCode;
    uint8 bestMove;
    score_t score, maxScore, lower;

    tree->nodeCount++;
    if (depth <= 0)
    {
        //return EvalPosTable(own, opp);
        return Evaluate(tree->eval, tree->nbEmpty);
    }

    if (tree->useHash == 1 && depth >= tree->hashDepth)
    {
        hashData = GetHashData(tree->table, tree->stones, depth, &hashCode);
        if (hashData != NULL && CutWithHash(hashData, &alpha, &beta, &score))
            return score;
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
            return WinJudge(tree->stones);
        }
        else
        {
            // 手番を入れ替えて探索続行
            SearchPassMid(tree);
            maxScore = -MidAlphaBeta(tree, -beta, -alpha, depth, true);
            SearchPassMid(tree);
            bestMove = PASS_INDEX;
        }
    }
    else
    {
        if (depth >= tree->orderDepth)
        {
            EvaluateMoveList(tree, &moveList, tree->stones, hashData);
            //SortMoveList(&moveList);
            NextSearch = MidAlphaBeta;
        }
        else
        {
            NextSearch = MidAlphaBetaDeep;
        }
        maxScore = -Const::MAX_VALUE;
        lower = alpha;
        // 打つ手がある時, 良い手から並べ替えつつループ
        //for (move = moveList.moves->next; move != NULL; move = move->next)
        for (move = NextBestMoveWithSwap(moveList.moves); move != NULL; move = NextBestMoveWithSwap(move))
        {
            SearchUpdateMid(tree, move);
            {
                // 子ノードを探索
                score = -NextSearch(tree, -beta, -lower, depth - 1, false);
            }
            SearchRestoreMid(tree, move);

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
        SaveHashData(tree->table, hashCode, tree->stones, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}

score_t MidAlphaBetaDeep(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed)
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
        return Evaluate(tree->eval, tree->nbEmpty);
    }

    if (tree->useHash == 1 && depth >= tree->hashDepth)
    {
        hashData = GetHashData(tree->table, tree->stones, depth, &hashCode);
        if (hashData != NULL && CutWithHash(hashData, &alpha, &beta, &score))
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
            return WinJudge(tree->stones);
        }
        else
        {
            // 手番を入れ替えて探索続行
            bestMove = PASS_INDEX;
            SearchPassMid(tree);
            maxScore = -MidAlphaBetaDeep(tree, -beta, -alpha, depth, true);
            SearchPassMid(tree);
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
            flip = CalcFlipOptimized(tree->stones, posIdx);

            SearchUpdateMidDeep(tree, pos, flip);
            {
                // 子ノードを探索
                score = -MidAlphaBetaDeep(tree, -beta, -lower, depth - 1, false);
            }
            SearchRestoreMidDeep(tree, pos, flip);

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
        SaveHashData(tree->table, hashCode, tree->stones, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}
