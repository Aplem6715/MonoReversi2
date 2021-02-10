#include "mid.h"
#include "mpc.h"
#include "hash.h"
#include "moves.h"
#include "../ai/eval.h"
#include "../bit_operation.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

score_t MidNullWindow(SearchTree *tree, const score_t beta, unsigned char depth, unsigned char passed);

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

bool NullWindowMultiProbCut(SearchTree *tree, const score_t alpha, const uint8 depth, score_t *score)
{
    int i;
    uint8 shallowDepth;
    score_t beta = alpha + 1;
    score_t shallowScore;
    double thresh;
    long bound;
    const MPCPair *mpc;

    if (depth >= MPC_DEEP_MIN && depth <= MPC_DEEP_MAX && tree->nbMpcNested < MPC_NEST_MAX)
    {
        //thresh = MPC_T[tree->nbMpcNested];
        thresh = MPC_DEPTH_T[depth];
        for (i = 0; i < MPC_NB_TRY; i++)
        {
            mpc = &mpcPairs[tree->nbEmpty][depth - MPC_DEEP_MIN][i];
            shallowDepth = mpc->shallowDepth;
            if (shallowDepth > 0)
            {
                bound = lround((thresh * mpc->std + beta - mpc->bias) / mpc->slope);
                if (bound < SCORE_MAX)
                {
                    tree->nbMpcNested++;
                    {
                        shallowScore = MidNullWindow(tree, (score_t)bound, shallowDepth, 0);
                    }
                    tree->nbMpcNested--;
                    if (shallowScore >= bound)
                    {
                        *score = beta;
                        return 1;
                    }
                }

                bound = lround((-thresh * mpc->std + alpha - mpc->bias) / mpc->slope);
                if (bound > SCORE_MIN)
                {
                    tree->nbMpcNested++;
                    {
                        shallowScore = MidNullWindow(tree, (score_t)bound + 1, shallowDepth, 0);
                    }
                    tree->nbMpcNested--;
                    if (shallowScore <= bound)
                    {
                        *score = alpha;
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
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
            maxScore = -MidAlphaBetaDeep(tree, -beta, -alpha, depth, true);
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

    if (tree->useHash == 1 && depth >= tree->hashDepth && tree->nbMpcNested == 0)
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

    if (tree->useHash == 1 && depth >= tree->hashDepth && tree->nbMpcNested == 0)
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
            maxScore = -MidNullWindowDeep(tree, -alpha, depth, true);
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

    if (tree->useHash == 1 && depth >= tree->hashDepth && tree->nbMpcNested == 0)
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

    if (tree->useHash == 1 && depth >= tree->hashDepth)
    {
        hashData = HashTableGetData(tree->table, tree->stones, depth, &hashCode);
        if (hashData != NULL && IsHashCutNullWindow(hashData, depth, alpha, &score))
            return score;
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
            maxScore = -NextNullSearch(tree, -alpha, depth, true);
            SearchPassMid(tree);
            bestMove = PASS_INDEX;
        }
    }
    else
    {

        if (tree->useMPC && NullWindowMultiProbCut(tree, alpha, depth, &score))
        {
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

    if (tree->useHash == 1 && depth >= tree->hashDepth && tree->nbMpcNested == 0)
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
            alpha = -NextSearch(tree, -in_beta, -in_alpha, depth, true);
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

    if (tree->useHash == 1 && depth >= tree->hashDepth && tree->nbMpcNested == 0)
    {
        HashTableRegist(tree->table, hashCode, tree->stones, bestMove, depth, in_alpha, in_beta, alpha);
    }

    assert(tree->nbMpcNested == 0);
    return alpha;
}

uint8 MidPVSRoot(SearchTree *tree, MoveList *moveList, uint8 depth, score_t *scoreOut, uint8 *secondMoveOut)
{
    SearchFunc_t NextSearch;
    score_t alpha, beta, score;
    uint8 foundPV = 0;
    uint8 bestMove = NOMOVE_INDEX;
    Move *move;

    if (depth >= tree->pvsDepth)
    {
        NextSearch = MidPVS;
    }
    else
    {
        NextSearch = MidAlphaBeta;
    }

    alpha = SCORE_MIN - 1;
    beta = SCORE_MAX + 1;

    for (move = NextBestMoveWithSwap(moveList->moves); move != NULL; move = NextBestMoveWithSwap(move))
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

        if (score > alpha) // alphaを上回る着手を発見したら
        {
            alpha = score;
            *secondMoveOut = bestMove;
            bestMove = move->posIdx;
            foundPV = 1; // PVを発見した！
        }
    } // end of moves loop
    *scoreOut = alpha;
    return bestMove;
}

uint8 MidRoot(SearchTree *tree, uint8 choiceSecond)
{
    MoveList moveList;
    uint8 startDepth, endDepth;
    uint8 bestMove, secondMove;
    uint8 tmpDepth;
    uint8 depths[10];
    uint8 nDepths;

    CreateMoveList(&moveList, tree->stones);
    EvaluateMoveList(tree, &moveList, tree->stones, NULL); // 着手の事前評価
    assert(moveList.nbMoves > 0);

    startDepth = 4;
    endDepth = tree->depth;

    nDepths = 0;
    for (tmpDepth = endDepth; tmpDepth >= startDepth; tmpDepth -= (int)sqrt(tmpDepth))
    {
        depths[nDepths] = tmpDepth;
        nDepths++;
    }

    // 反転
    int i = 0;
    for (i = 0; i < nDepths / 2; i++)
    {
        tmpDepth = depths[i];
        depths[i] = depths[(nDepths - 1) - i];
        depths[(nDepths - 1) - i] = tmpDepth;
    }

    for (i = 0; i < nDepths; i++)
    {
        bestMove = MidPVSRoot(tree, &moveList, depths[i], &tree->score, &secondMove);
    }
    bestMove = MidPVSRoot(tree, &moveList, endDepth, &tree->score, &secondMove);

    if (choiceSecond && moveList.nbMoves >= 2)
    {
        return secondMove;
    }

    return bestMove;
}

uint8 MidRootWithMpcLog(SearchTree *deepTree, SearchTree *shallowTree, FILE *logFile, int matchIdx, uint8 shallow, uint8 deep, uint8 minimumDepth)
{
    MoveList moveList;
    uint8 bestMove, secondMove;

    CreateMoveList(&moveList, deepTree->stones);
    CreateMoveList(&moveList, shallowTree->stones);
    EvaluateMoveList(deepTree, &moveList, deepTree->stones, NULL);       // 着手の事前評価
    EvaluateMoveList(shallowTree, &moveList, shallowTree->stones, NULL); // 着手の事前評価
    assert(moveList.nbMoves > 0);

    deepTree->isEndSearch = 0;
    deepTree->pvsDepth = deepTree->midPvsDepth;
    deepTree->orderDepth = deepTree->pvsDepth;
    deepTree->hashDepth = deepTree->pvsDepth - 1;
    shallowTree->isEndSearch = 0;
    shallowTree->pvsDepth = shallowTree->midPvsDepth;
    shallowTree->orderDepth = shallowTree->pvsDepth;
    shallowTree->hashDepth = shallowTree->pvsDepth - 1;

    // 浅い探索をしてスコアを記録
    shallowTree->depth = shallow;
    if (shallow < deepTree->nbEmpty)
    {
        printf("Searching depth:%d \r", shallow);
        bestMove = MidPVSRoot(shallowTree, &moveList, shallow, &shallowTree->score, &secondMove);
        if (abs(shallowTree->score) != SCORE_MAX)
            fprintf(logFile, "%d,%d,%d,%d\n", matchIdx, shallowTree->nbEmpty, shallow, shallowTree->score);
    }

    // 深い探索をしてスコアを記録
    deepTree->depth = deep;
    if (deep < deepTree->nbEmpty)
    {
        printf("Searching depth:%d \r", deep);
        bestMove = MidPVSRoot(deepTree, &moveList, deep, &deepTree->score, &secondMove);
        if (abs(deepTree->score) != SCORE_MAX)
            fprintf(logFile, "%d,%d,%d,%d\n", matchIdx, deepTree->nbEmpty, deep, deepTree->score);
    }

    return bestMove;
}
