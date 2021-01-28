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

    HashData *hashData = NULL;
    uint64_t mob, pos, flip, hashCode;
    score_t score, maxScore, lower;
    uint8 posIdx;
    uint8 bestMove;

    tree->nodeCount++;
    if (depth <= 0)
    {
        return Evaluate(tree->eval, tree->nbEmpty);
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
            // パスして探索続行
            SearchPassMid(tree);
            maxScore = -MidAlphaBetaDeep(tree, -beta, -alpha, depth - 1, true);
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
        return Evaluate(tree->eval, tree->nbEmpty);
    }

    CreateMoveList(&moveList, tree->stones);

    if (moveList.nbMoves <= 0) // 手があるか
    {
        if (passed == 1) // 2連続パスなら終了
        {
            bestMove = NOMOVE_INDEX;
            return WinJudge(tree->stones); // 勝敗判定
        }
        else
        { // 手番を入れ替えて探索続行
            SearchPassMid(tree);
            maxScore = -MidAlphaBeta(tree, -beta, -alpha, depth - 1, true);
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
            SearchUpdateMid(tree, move); // 子ノードを探索
            {
                score = -NextSearch(tree, -beta, -lower, depth - 1, false);
            }
            SearchRestoreMid(tree, move);

            if (score > maxScore)
            {
                maxScore = score;
                bestMove = move->posIdx;

                if (score >= beta) // 上限突破したら
                {
                    tree->nbCut++;
                    break; // 探索終了（カット）
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
score_t MidNullWindowDeep(SearchTree *tree, const score_t beta, unsigned char depth, unsigned char passed)
{
    const score_t alpha = beta - 1;
    HashData *hashData = NULL;
    uint64_t mob, pos, flip, hashCode;
    score_t score, maxScore;
    uint8 posIdx;
    uint8 bestMove;

    tree->nodeCount++;
    if (depth <= 0)
    {
        return Evaluate(tree->eval, tree->nbEmpty);
    }

    mob = CalcMobility(tree->stones);
    if (mob == 0) // 手があるか
    {
        if (passed == 1)
        { // 2連続パスなら終了
            bestMove = NOMOVE_INDEX;
            return WinJudge(tree->stones); // 勝敗判定
        }
        else
        { // パスして探索続行
            SearchPassMid(tree);
            maxScore = -MidNullWindowDeep(tree, -alpha, depth - 1, true);
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

        maxScore = -Const::MAX_VALUE;
        while (mob != 0)
        {
            // 着手位置・反転位置を取得
            pos = GetLSB(mob);
            mob ^= pos;
            posIdx = CalcPosIndex(pos);
            flip = CalcFlip(tree->stones, posIdx);

            SearchUpdateMidDeep(tree, pos, flip);
            { // 子ノードを探索
                score = -MidNullWindowDeep(tree, -alpha, depth - 1, false);
            }
            SearchRestoreMidDeep(tree, pos, flip);

            if (score > maxScore)
            {
                maxScore = score;
                bestMove = posIdx;
                if (score >= beta)
                {
                    break; //探索終了
                }
            }
        }
    } // end of if(mob == 0) else

    if (tree->useHash == 1)
    {
        HashTableRegist(tree->table, hashCode, tree->stones, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}

// https://www.chessprogramming.org/Principal_Variation_Search
score_t MidNullWindow(SearchTree *tree, const score_t beta, unsigned char depth, unsigned char passed)
{
    SearchFuncNullWindow_t NextNullSearch;
    HashData *hashData = NULL;
    MoveList moveList;
    Move *move;
    uint64_t hashCode;
    uint8 bestMove;
    score_t score, maxScore;
    const score_t alpha = beta - 1;

    tree->nodeCount++;
    if (depth <= 0)
    {
        return Evaluate(tree->eval, tree->nbEmpty);
    }

    if (depth - 1 >= tree->orderDepth)
    {
        NextNullSearch = MidNullWindow;
    }
    else
    {
        NextNullSearch = MidNullWindowDeep;
    }

    CreateMoveList(&moveList, tree->stones);
    if (moveList.nbMoves <= 0) // 手があるか
    {
        if (passed == 1)
        { // 2連続パスなら終了
            bestMove = NOMOVE_INDEX;
            return WinJudge(tree->stones); // 勝敗判定
        }
        else
        { // パスして探索続行
            SearchPassMid(tree);
            maxScore = -NextNullSearch(tree, -alpha, depth - 1, true);
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

        EvaluateMoveList(tree, &moveList, tree->stones, hashData);

        maxScore = -Const::MAX_VALUE;

        for (move = NextBestMoveWithSwap(moveList.moves); move != NULL; move = NextBestMoveWithSwap(move))
        {
            SearchUpdateMid(tree, move);
            { // 子ノードを探索
                score = -NextNullSearch(tree, -alpha, depth - 1, false);
            }
            SearchRestoreMid(tree, move);

            if (score > maxScore)
            {
                maxScore = score;
                bestMove = move->posIdx;
                if (score >= beta)
                {
                    break; //探索終了
                }
            }
        }
    } // end of if(moveList.nbMoves > 0)

    if (tree->useHash == 1)
    {
        HashTableRegist(tree->table, hashCode, tree->stones, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}

//
/**
 * @brief 中盤のPVS探索
 * 
 *  https://www.chessprogramming.org/Principal_Variation_Search
 * 
 * @param tree 探索木用データ
 * @param in_alpha アルファ値
 * @param in_beta ベータ値
 * @param depth 残りの探索深度
 * @param passed パスされたかどうか
 * @return score_t 最善手のスコア
 */
score_t MidPVS(SearchTree *tree, const score_t in_alpha, const score_t in_beta, const unsigned char depth, const unsigned char passed)
{
    SearchFunc_t NextSearch;
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

    if (depth - 1 >= tree->pvsDepth)
    {
        NextSearch = MidPVS;
    }
    else
    {
        NextSearch = MidAlphaBeta;
    }

    CreateMoveList(&moveList, tree->stones); // 着手リストを作成

    if (moveList.nbMoves <= 0)
    { // 手があるか
        if (passed == 1)
        { // 2連続パスなら終了
            bestMove = NOMOVE_INDEX;
            return WinJudge(tree->stones); // 勝敗判定
        }
        else
        { // パスして探索続行
            SearchPassMid(tree);
            alpha = -NextSearch(tree, -in_beta, -in_alpha, depth - 1, true);
            SearchPassMid(tree);
            bestMove = PASS_INDEX;
        }
    }
    else
    { // 着手可能なとき
        alpha = in_alpha;
        beta = in_beta;

        if (tree->useHash == 1 && depth >= tree->hashDepth)
        { // ハッシュの記録をもとにカット/探索範囲の縮小
            hashData = HashTableGetData(tree->table, tree->stones, depth, &hashCode);
            // PVノードはカットしない(性能も殆ど変わらなかった)
            //if (hashData != NULL && IsHashCut(hashData, depth, &alpha, &beta, &score))
            //    return score;
        }

        EvaluateMoveList(tree, &moveList, tree->stones, hashData); // 着手の事前評価

        for (move = NextBestMoveWithSwap(moveList.moves); move != NULL; move = NextBestMoveWithSwap(move))
        { // すべての着手についてループ
            SearchUpdateMid(tree, move);
            if (!foundPV)
            {                                                               // PVが見つかっていない
                score = -NextSearch(tree, -beta, -alpha, depth - 1, false); // 通常探索
            }
            else
            {                                                           // PVが見つかっている
                score = -MidNullWindow(tree, -alpha, depth - 1, false); // 最善かどうかチェック 子ノードをNull Window探索
                if (score > alpha)                                      // 予想が外れていたら
                {
                    score = -NextSearch(tree, -beta, -alpha, depth - 1, false); // 通常のWindowで再探索
                }
            }
            SearchRestoreMid(tree, move);

            if (score >= beta) // 上限突破したら
            {
                alpha = score;
                tree->nbCut++;
                break; // 探索終了（カット）
            }

            if (score > alpha) // alphaを上回る着手を発見したら
            {
                alpha = score;
                bestMove = move->posIdx;
                foundPV = 1; // PVを発見した！
            }
        } // end of moves loop
    }

    if (tree->useHash == 1)
    {
        HashTableRegist(tree->table, hashCode, tree->stones, bestMove, depth, in_alpha, in_beta, alpha);
    }
    return alpha;
}

uint8 MidRoot(SearchTree *tree, uint8 choiceSecond)
{
    SearchFunc_t NextSearch;
    score_t score, maxScore = -Const::MAX_VALUE;
    score_t alpha, beta;
    uint8 bestMove = NOMOVE_INDEX, secondMove = NOMOVE_INDEX;
    uint8 foundPV = 0;
    uint8 depth = tree->depth;
    MoveList moveList;
    Move *move;

    if (depth - 1 >= tree->pvsDepth)
    {
        NextSearch = MidPVS;
    }
    else
    {
        NextSearch = MidAlphaBeta;
    }

    alpha = -Const::MAX_VALUE;
    beta = Const::MAX_VALUE;

    CreateMoveList(&moveList, tree->stones);
    EvaluateMoveList(tree, &moveList, tree->stones, NULL); // 着手の事前評価
    assert(moveList.nbMoves > 0);

    for (move = NextBestMoveWithSwap(moveList.moves); move != NULL; move = NextBestMoveWithSwap(move))
    { // すべての着手についてループ
        SearchUpdateMid(tree, move);
        if (!foundPV)
        {                                                           // PVが見つかっていない
            score = -NextSearch(tree, -beta, -alpha, depth, false); // 通常探索
        }
        else
        {                                                       // PVが見つかっている
            score = -MidNullWindow(tree, -alpha, depth, false); // 最善かどうかチェック 子ノードをNull Window探索
            if (score > alpha)                                  // 予想が外れていたら
            {
                score = -NextSearch(tree, -beta, -alpha, depth, false); // 通常のWindowで再探索
            }
        }
        SearchRestoreMid(tree, move);

        if (score >= beta) // 上限突破したら
        {
            alpha = score;
            tree->nbCut++;
            break; // 探索終了（カット）
        }

        if (score > alpha) // alphaを上回る着手を発見したら
        {
            alpha = score;
            secondMove = bestMove;
            bestMove = move->posIdx;
            foundPV = 1; // PVを発見した！
        }
    } // end of moves loop

    tree->score = alpha;

    if (choiceSecond == 1 && secondMove != NOMOVE_INDEX)
    {
        return secondMove;
    }
    return bestMove;
}
