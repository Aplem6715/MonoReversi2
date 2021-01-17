#include "mid.h"
#include "hash.h"
#include "moves.h"
#include "../ai/eval.h"
#include "../bit_operation.h"
#include <assert.h>

inline float WinJudge(const uint64 own, const uint64 opp)
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

float PVS(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed)
{
    uint8 bestMove;
    uint64 pos, hashCode;
    SearchFunc_t NextSearch;
    MoveList moveList;
    Move *move;
    HashData *hashData = NULL;
    float score, maxScore, lower;

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
            EvaluateMoveList(tree, &moveList, own, opp, alpha, hashData);
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

float MidAlphaBetaDeep(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed)
{

    assert(depth <= tree->orderDepth);

    uint64 mob, pos, rev, hashCode;
    uint8 posIdx;
    uint8 bestMove;
    HashData *hashData = NULL;
    float score, maxScore, lower;

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
            UpdateEvalPass(tree->eval);
            maxScore = -MidAlphaBetaDeep(tree, opp, own, -beta, -alpha, depth, true);
            UpdateEvalPass(tree->eval);
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
            posIdx = CalcPosIndex(pos);
            mob ^= pos;
            rev = CalcFlip(own, opp, pos);

            UpdateEval(tree->eval, posIdx, rev);
            {
                // 子ノードを探索
                score = -MidAlphaBetaDeep(tree, opp ^ rev, own ^ rev ^ pos, -beta, -lower, depth - 1, false);
            }
            UndoEval(tree->eval, posIdx, rev);

            if (score > maxScore)
            {
                maxScore = score;
                bestMove = CalcPosIndex(pos);

                // 上限突破したら
                if (score >= beta)
                {
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

float MidAlphaBeta(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed)
{
    uint8 bestMove;
    uint64 pos, hashCode;
    SearchFunc_t NextSearch;
    MoveList moveList;
    Move *move;
    HashData *hashData = NULL;
    float score, maxScore, lower;

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
            EvaluateMoveList(tree, &moveList, own, opp, alpha, hashData);
            SortMoveList(&moveList);
            NextSearch = MidAlphaBeta;
        }
        else
        {
            NextSearch = MidAlphaBetaDeep;
        }
        maxScore = -Const::MAX_VALUE;
        lower = alpha;
        // 打つ手がある時, 良い手から並べ替えつつループ
        for (move = moveList.moves->next; move != NULL; move = move->next)
        {
            // 着手位置・反転位置を取得
            pos = CalcPosBit(move->posIdx);

            UpdateEval(tree->eval, move->posIdx, move->flip);
            {
                // 子ノードを探索
                score = -NextSearch(tree, opp ^ move->flip, own ^ move->flip ^ pos, -beta, -lower, depth - 1, false);
            }
            UndoEval(tree->eval, move->posIdx, move->flip);

            if (score > maxScore)
            {
                maxScore = score;
                bestMove = move->posIdx;

                // 上限突破したら
                if (score >= beta)
                {
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
