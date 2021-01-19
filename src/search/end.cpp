#include "end.h"
#include "hash.h"
#include "moves.h"
#include "../ai/eval.h"
#include "../bit_operation.h"
#include <assert.h>

inline float Judge(const SearchTree *tree, const uint64 own, const uint64 opp)
{
    const uint8 nbOwn = CountBits(own);
    const uint8 nbOpp = 64 - tree->eval->nbEmpty - nbOwn;
    return (float)(nbOwn - nbOpp);
}

float EndAlphaBetaDeep(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed)
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
        return Judge(tree, own, opp);
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
            return Judge(tree, own, opp);
        }
        else
        {
            // 手番を入れ替えて探索続行
            bestMove = PASS_INDEX;
            UpdateEvalPass(tree->eval);
            maxScore = -EndAlphaBetaDeep(tree, opp, own, -beta, -alpha, depth, true);
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
            rev = CalcFlipOptimized(own, opp, posIdx);

            UpdateEval(tree->eval, posIdx, rev);
            {
                // 子ノードを探索
                score = -EndAlphaBetaDeep(tree, opp ^ rev, own ^ rev ^ pos, -beta, -lower, depth - 1, false);
            }
            UndoEval(tree->eval, posIdx, rev);

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
        SaveHashData(tree->table, hashCode, own, opp, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}

float EndAlphaBeta(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed)
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
        return Judge(tree, own, opp);
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
            return Judge(tree, own, opp);
        }
        else
        {
            // 手番を入れ替えて探索続行
            UpdateEvalPass(tree->eval);
            maxScore = -EndAlphaBeta(tree, opp, own, -beta, -alpha, depth, true);
            UpdateEvalPass(tree->eval);
            bestMove = PASS_INDEX;
        }
    }
    else
    {
        if (depth >= tree->orderDepth)
        {
            EvaluateMoveList(tree, &moveList, own, opp, hashData);
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