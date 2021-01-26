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
            flip = CalcFlip(tree->stones, posIdx);

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
        HashTableRegist(tree->table, hashCode, tree->stones, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
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

        if (tree->useHash == 1 && depth >= tree->hashDepth)
        {
            hashData = HashTableGetData(tree->table, tree->stones, depth, &hashCode);
            if (hashData != NULL && IsHashCut(hashData, depth, &alpha, &beta, &score))
                return score;
        }
        if (depth >= tree->orderDepth)
        {
            EvaluateMoveList(tree, &moveList, tree->stones, hashData);
            NextSearch = MidAlphaBeta;
        }
        else
        {
            NextSearch = MidAlphaBetaDeep;
        }
        maxScore = -Const::MAX_VALUE;
        lower = alpha;

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
        HashTableRegist(tree->table, hashCode, tree->stones, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}

// https://www.chessprogramming.org/Principal_Variation_Search
score_t MidNullWindow(SearchTree *tree, const score_t alpha, unsigned char depth, unsigned char passed)
{
    SearchFunc_t NextSearch;
    HashData *hashData = NULL;
    MoveList moveList;
    Move *move;
    uint64_t hashCode;
    uint8 bestMove;
    score_t score, maxScore, lower;
    score_t beta = alpha + 1;

    tree->nodeCount++;
    if (depth <= 0)
    {
        return Evaluate(tree->eval, tree->nbEmpty);
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
            maxScore = -MidNullWindow(tree, -beta, depth, true);
            SearchPassMid(tree);
            bestMove = PASS_INDEX;
        }
    }
    else
    {
        if (tree->useHash == 1 && depth >= tree->hashDepth)
        {
            hashData = HashTableGetData(tree->table, tree->stones, depth, &hashCode);
            if (hashData != NULL && IsHashCutNullWindow(hashData, depth, alpha, &score))
                return score;
        }
        if (depth >= tree->orderDepth)
        {
            EvaluateMoveList(tree, &moveList, tree->stones, hashData);
        }
        maxScore = -Const::MAX_VALUE;
        lower = alpha;

        for (move = NextBestMoveWithSwap(moveList.moves); move != NULL; move = NextBestMoveWithSwap(move))
        {
            SearchUpdateMid(tree, move);
            {
                // 子ノードを探索
                score = -MidNullWindow(tree, -beta, depth - 1, false);
            }
            SearchRestoreMid(tree, move);

            if (score >= beta)
            {
                return beta;
            }
        }
    }

    if (tree->useHash == 1)
    {
        HashTableRegist(tree->table, hashCode, tree->stones, bestMove, depth, alpha, beta, maxScore);
    }
    return alpha;
}

// https://www.chessprogramming.org/Principal_Variation_Search
score_t MidPVS(SearchTree *tree, const score_t in_alpha, const score_t in_beta, const unsigned char depth, const unsigned char passed)
{
    HashData *hashData = NULL;
    MoveList moveList;
    Move *move;
    uint64_t hashCode;
    uint8 bestMove;
    uint8 foundPV = 0;
    score_t score, alpha, beta;

    tree->nodeCount++;
    if (depth <= 0)
    {
        return Evaluate(tree->eval, tree->nbEmpty);
    }

    // 着手リストを作成
    CreateMoveList(&moveList, tree->stones);

    // 手があるか
    if (moveList.nbMoves <= 0)
    {
        if (passed == 1)
        {
            // 2連続パスなら終了
            bestMove = NOMOVE_INDEX;
            // 勝敗判定
            return WinJudge(tree->stones);
        }
        else
        {
            // パスして探索続行
            SearchPassMid(tree);
            alpha = -MidPVS(tree, -in_beta, -in_alpha, depth, true);
            SearchPassMid(tree);
            bestMove = PASS_INDEX;
        }
    }
    else
    { // 着手可能なとき
        alpha = in_alpha;
        beta = in_beta;

        // ハッシュの記録をもとにカット/探索範囲の縮小
        if (tree->useHash == 1 && depth >= tree->hashDepth)
        {
            hashData = HashTableGetData(tree->table, tree->stones, depth, &hashCode);
            if (hashData != NULL && IsHashCut(hashData, depth, &alpha, &beta, &score))
                return score;
        }

        // 着手の事前評価
        EvaluateMoveList(tree, &moveList, tree->stones, hashData);

        // すべての着手についてループ
        for (move = NextBestMoveWithSwap(moveList.moves); move != NULL; move = NextBestMoveWithSwap(move))
        {
            SearchUpdateMid(tree, move);
            if (foundPV)
            { // PVが見つかっている
                // 最善かどうかチェック 子ノードをNull Window探索
                score = -MidNullWindow(tree, -alpha - 1, depth - 1, false);
                if (alpha < score)
                { // 予想が外れていたら
                    // 通常のWindowで再探索
                    score = -MidPVS(tree, -beta, -alpha, depth - 1, false);
                }
            }
            else
            { // PVが見つかっていない
                // 通常探索
                score = -MidPVS(tree, -beta, -alpha, depth - 1, false);
            }
            SearchRestoreMid(tree, move);

            // 上限突破したら
            if (score >= beta)
            {
                tree->nbCut++;
                // 探索終了（カット）
                break;
            }

            if (score > alpha)
            {
                alpha = score;
                bestMove = move->posIdx;
                // PVを発見した！
                foundPV = 1;
            }
        }
        /*
        SearchUpdateMid(tree, move);
        {
            score = -MidPVS(tree, -beta, -alpha, depth - 1, false);
        }
        SearchRestoreMid(tree, move);
        if (score >= beta)
        {
            // betaカット
            // これ以上の探索は必要ない
            maxScore = score;
            bestMove = move->posIdx;
        }
        else
        { // 初回探索でbetaカットされなかった
            // 探索範囲をできる限り狭める
            if (score > alpha)
                alpha = score;
            // 打つ手がある時, 良い手から並べ替えつつループ
            for (move = NextBestMoveWithSwap(move); move != NULL; move = NextBestMoveWithSwap(move))
            {
                SearchUpdateMid(tree, move);
                {
                    // 最善かどうかチェック 子ノードをNull Window探索
                    score = -MidNullWindow(tree, -alpha - 1, depth - 1, false);
                    if (alpha < score && score < beta)
                    { // 予想が外れていたら
                        // 通常の探索
                        score = MidPVS(tree, -beta, -alpha, depth - 1, false);
                    }
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
                    else if (maxScore > alpha)
                    {
                        alpha = maxScore;
                    }
                }
            }
        }
        */
    }

    if (tree->useHash == 1)
    {
        HashTableRegist(tree->table, hashCode, tree->stones, bestMove, depth, in_alpha, in_beta, alpha);
    }
    return alpha;
}
