#include "mid.h"
#include "hash.h"
#include "moves.h"
#include "../ai/eval.h"
#include "../bit_operation.h"
#include <assert.h>

inline score_t WinJudge(const uint64_t own, const uint64_t opp)
{
    uint8 ownCnt = CountBits(own);
    uint8 oppCnt = CountBits(opp);
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

score_t PVS(SearchTree *tree, uint64_t own, uint64_t opp, score_t alpha, score_t beta, unsigned char depth, unsigned char passed)
{
    uint8 bestMove;
    uint64_t pos, hashCode;
    SearchFunc_t NextSearch;
    MoveList moveList;
    Move *move;
    HashData *hashData = NULL;
    score_t score, maxScore, lower;

    tree->nodeCount++;
    if (depth <= 0)
    {
        //return EvalPosTable(own, opp);
        return EvalNNet(tree->eval);
    }

    if (tree->useHash == 1 && depth >= tree->hashDepth)
    {
        hashData = GetHashData(tree->table, own, opp, depth, &hashCode);
        if (hashData != NULL && CutWithHash(hashData, &alpha, &beta, &score))
            return score;
    }

    CreateMoveList(&moveList, own, opp);

    // 手があるか
    if (moveList.nbMoves <= 0)
    {
        // 2連続パスなら終了
        if (passed == 1)
        {
            bestMove = NOMOVE_INDEX;
            // 勝敗判定
            return WinJudge(own, opp);
        }
        else
        {
            // 手番を入れ替えて探索続行
            UpdateEvalPass(tree->eval);
            maxScore = -MidAlphaBeta(tree, opp, own, -beta, -alpha, depth, true);
            UpdateEvalPass(tree->eval);
            bestMove = PASS_INDEX;
        }
    }
    else
    {
        if (depth >= tree->orderDepth)
        {
            EvaluateMoveList(tree, &moveList, own, opp, hashData);
            SortMoveList(&moveList);
            NextSearch = PVS;
        }
        else
        {
            NextSearch = MidAlphaBetaDeep;
        }
        maxScore = -Const::MAX_VALUE;
        lower = alpha;

        move = moveList.moves->next;
        pos = CalcPosBit(move->posIdx);
        UpdateEval(tree->eval, move->posIdx, move->flip);
        {
            // 最善手候補を通常探索
            maxScore = score = -PVS(tree, opp ^ move->flip, own ^ move->flip ^ pos, -beta, -lower, depth - 1, 0);
        }
        UndoEval(tree->eval, move->posIdx, move->flip);
        if (beta <= score)
            return score;
        if (lower < score)
            lower = score;

        // 打つ手がある時, 良い手から並べ替えつつループ
        for (move = move->next; move != NULL; move = move->next)
        {
            // 着手位置・反転位置を取得
            pos = CalcPosBit(move->posIdx);

            UpdateEval(tree->eval, move->posIdx, move->flip);
            {
                // Null Window Search
                score = -NextSearch(tree, opp ^ move->flip, own ^ move->flip ^ pos, -lower - 1, -lower, depth - 1, 0);

                if (score >= beta)
                {
                    maxScore = score;
                    bestMove = move->posIdx;
                    break;
                }
                if (alpha < score)
                {
                    lower = score;

                    // 通常探索
                    {
                        score = -NextSearch(tree, opp ^ move->flip, own ^ move->flip ^ pos, -beta, -lower, depth - 1, 0);
                    }

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
            UndoEval(tree->eval, move->posIdx, move->flip);
        }
    }

    if (tree->useHash == 1)
    {
        SaveHashData(tree->table, hashCode, own, opp, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}

score_t MidAlphaBeta(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed)
{
    SearchFunc_t NextSearch;
    HashData *hashData = NULL;
    MoveList moveList;
    Move *move;
    uint64_t pos, hashCode;
    uint8 bestMove;
    score_t score, maxScore, lower;

    tree->nodeCount++;
    if (depth <= 0)
    {
        //return EvalPosTable(own, opp);
        return EvalNNet(tree->eval);
    }

    if (tree->useHash == 1 && depth >= tree->hashDepth)
    {
        hashData = GetHashData(tree->table, own, opp, depth, &hashCode);
        if (hashData != NULL && CutWithHash(hashData, &alpha, &beta, &score))
            return score;
    }

    CreateMoveList(&moveList, own, opp);

    // 手があるか
    if (moveList.nbMoves <= 0)
    {
        // 2連続パスなら終了
        if (passed == 1)
        {
            bestMove = NOMOVE_INDEX;
            // 勝敗判定
            return WinJudge(own, opp);
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
            EvaluateMoveList(tree, &moveList, own, opp, hashData);
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
        SaveHashData(tree->table, hashCode, own, opp, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}

score_t MidAlphaBetaDeep(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, unsigned char passed)
{

    assert(depth <= tree->orderDepth);

    uint64_t mob, pos, rev, hashCode;
    uint8 posIdx;
    uint8 bestMove;
    HashData *hashData = NULL;
    score_t score, maxScore, lower;

    tree->nodeCount++;
    if (depth <= 0)
    {
        //return EvalPosTable(own, opp);
        //return EvalTinyDnn(tree, tree->nbEmpty);
        return EvalNNet(tree->eval);
    }

    if (tree->useHash == 1 && depth >= tree->hashDepth)
    {
        hashData = GetHashData(tree->table, own, opp, depth, &hashCode);
        if (hashData != NULL && CutWithHash(hashData, &alpha, &beta, &score))
            return score;
    }

    mob = CalcMobility(own, opp);

    // 手があるか
    if (mob == 0)
    {
        // 2連続パスなら終了
        if (passed == 1)
        {
            bestMove = NOMOVE_INDEX;
            // 勝敗判定
            return WinJudge(own, opp);
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

            SearchUpdateMidDeep(tree, pos);
            {
                // 子ノードを探索
                score = -MidAlphaBetaDeep(tree, -beta, -lower, depth - 1, false);
            }
            SearchRestoreMidDeep(tree, pos);

            if (score > maxScore)
            {
                maxScore = score;
                bestMove = CalcPosIndex(pos);

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
        SaveHashData(tree->table, hashCode, own, opp, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}
