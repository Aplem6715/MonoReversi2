/**
 * @file end.c
 * @author Daichi Sato
 * @brief 終盤探索機能の定義
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 * 終盤探索では終局まで探索を行い，最善進行と石差を計算する。
 * 最善進行を探索するため，どのアルゴリズムを用いても結果は同じにならなければいけない。
 * デバッグしやすくてありがたい。
 * 
 * メインとなるアルゴリズム：
 * ・NullWindowSearch(NWSと略す。αβ探索のWindowを1にして探索。高速にα以上or以下を判別できる)
 * ・PVS(NWSを用いて高速に探索する深さ優先探索アルゴリズム)
 * ・反復深化(だんだん深く探索)
 * ・TranspositionTable（置換表・ハッシュ）
 * ・MoveOrdering(探索順番の並び替え)
 */

#include <stdio.h>
#include "end.h"
#include "search.h"
#include "hash.h"
#include "moves.h"
#include "../ai/eval.h"
#include "../bit_operation.h"
#include "../const.h"
#include <assert.h>
#include <math.h>

/**
 * @brief 石差を計算する
 * 
 * @param tree 探索木
 * @return score_t 石差
 */
inline score_t Judge(const SearchTree *tree)
{
    const uint8 nbOwn = CountBits(tree->stones->own);
    const uint8 nbOpp = 64 - tree->nbEmpty - nbOwn;
    return (score_t)((nbOwn - nbOpp) * STONE_VALUE);
}

/**
 * @brief 残り1マス状態で最終石差を計算する
 * 
 *      
 * [nbOwn - nbOpp]は
 * nbOpp = 64 - nbOwn - 1 なので
 * nbOwn - nbOpp = nbOwn - (64 - nbOwn - 1) = [2*nbOwm - 64 + 1]
 * 
 * score = [nbOwn - nbOpp] - 2*nbFlip - 1(put)
 *       = [2*nbOwm - 64 + 1] - 2*nbFlip -1
 *       = [2*nbOwn - 64 + 1] - [(2*nbFlip + 1)]
 *       = [2*nbOwn - 64 + 1](パスでも不変)    [-(2*nbFlip + 1)]か[+(2*nbFlip + 1)]
 * 
 * @param stones 
 * @param alpha 
 * @return score_t 石差
 */
inline score_t SolveLast1(Stones *stones, const score_t alpha)
{
    score_t ownScore;
    uint64_t flips;
    uint64_t posBit = ~(stones->own | stones->opp);
    uint8 pos = PosIndexFromBit(posBit);
    uint8 nbFlips;
    assert(CountBits(posBit) == 1);

    // [2*nbOwn - 64]
    ownScore = 2 * CountBits(stones->own) - 64 + 1;

    flips = CalcFlip64(stones->own, stones->opp, pos);
    if (flips != 0)
    {
        nbFlips = CountBits(flips);
        // -[2*nbFlip]
        ownScore += 2 * nbFlips + 1;
    }
    else
    {
        if (ownScore < alpha)
        {
            return ownScore;
        }
        else
        {
            flips = CalcFlip64(stones->opp, stones->own, pos);
            if (flips != 0)
            {
                nbFlips = CountBits(flips);
                ownScore -= 2 * nbFlips + 1;
            }
        }
    }
    return ownScore;
}

inline uint8 CalcCost(uint64_t nbNodes)
{
    return (uint8)log2l((long double)nbNodes);
}

/**
 * @brief 終盤探索αβ法
 * 
 * @param tree 探索木
 * @param alpha アルファ値
 * @param beta ベータ値
 * @param depth 探索深度
 * @param passed パスされたか？
 * @return score_t 探索結果スコア
 */
score_t EndAlphaBeta(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, bool passed)
{
    uint8 bestMove;
    uint64_t hashCode;
    SearchFunc_t NextSearch;
    MoveList moveList;
    Move *move;
    HashData *hashData = NULL;
    score_t score, maxScore, lower;

    // 子ノード数（スタート時点でのノード数で初期化）
    uint64_t nbChildNode = tree->nodeCount;
    // 探索コスト
    uint8 cost;

    tree->nodeCount++;
    if (depth <= 0)
    {
        //return EvalPosTable(own, opp);
        return Judge(tree);
    }
    if (depth == 1)
    {
        return SolveLast1(tree->stones, alpha);
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
        if (tree->option.usePvHash == 1 && depth >= tree->pvHashDepth)
        {
            hashData = HashTableGetData(tree->pvTable, tree->stones, depth, &hashCode);
            if (hashData != NULL && IsHashCut(hashData, depth, &alpha, &beta, &score))
                return score;
        }
        if (depth >= tree->orderDepth)
        {
            EvaluateMoveList(tree, &moveList, tree->stones, alpha, hashData);
            //SortMoveList(&moveList);
            NextSearch = EndAlphaBeta;
        }
        else
        {
            NextSearch = EndAlphaBetaDeep;
        }
        maxScore = -MAX_VALUE;
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

    // 「現在のノード数」と「スタート時点でのノード数」の差分＝子ノード数
    nbChildNode = tree->nodeCount - nbChildNode;
    cost = CalcCost(nbChildNode);

    if (tree->option.usePvHash == 1 && depth >= tree->pvHashDepth)
    {
        HashTableRegist(tree->pvTable, hashCode, tree->stones, bestMove, cost, depth, alpha, beta, maxScore);
    }
    return maxScore;
}

/**
 * @brief 深い探索での終盤αβ法
 * 
 * Movelistの生成・評価を行なわず，ビット位置で処理する。
 * 高速な探索が可能
 * 
 * @param tree 探索木
 * @param alpha アルファ値
 * @param beta ベータ値
 * @param depth 探索深度
 * @param passed パスされたか
 * @return score_t 探索スコア
 */
score_t EndAlphaBetaDeep(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, bool passed)
{

    assert(depth <= tree->orderDepth);

    uint64_t mob, pos, flip, hashCode;
    uint8 posIdx;
    uint8 bestMove;
    HashData *hashData = NULL;
    score_t score, maxScore, lower;

    // 子ノード数（スタート時点でのノード数で初期化）
    uint64_t nbChildNode = tree->nodeCount;
    // 探索コスト
    uint8 cost;

    tree->nodeCount++;
    if (depth <= 0)
    {
        //return EvalPosTable(own, opp);
        //return EvalTinyDnn(tree, tree->nbEmpty);
        return Judge(tree);
    }
    if (depth == 1)
    {
        return SolveLast1(tree->stones, alpha);
    }

    if (tree->option.usePvHash == 1 && depth >= tree->pvHashDepth)
    {
        hashData = HashTableGetData(tree->pvTable, tree->stones, depth, &hashCode);
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
        maxScore = -MAX_VALUE;
        lower = alpha;
        // 打つ手がある時
        while (mob != 0)
        {
            // 着手位置・反転位置を取得
            pos = GetLSB(mob);
            mob ^= pos;
            posIdx = PosIndexFromBit(pos);
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

    // 「現在のノード数」と「スタート時点でのノード数」の差分＝子ノード数
    nbChildNode = tree->nodeCount - nbChildNode;
    cost = CalcCost(nbChildNode);

    if (tree->option.usePvHash == 1 && depth >= tree->pvHashDepth)
    {
        HashTableRegist(tree->pvTable, hashCode, tree->stones, bestMove, cost, depth, alpha, beta, maxScore);
    }
    return maxScore;
}

/**
 * @brief 深い深度での終盤NWS
 * 
 *  move orderingなしで高速なNWS
 * 
 * 参考
 * https://www.chessprogramming.org/Principal_Variation_Search
 * 
 * @param tree 探索木
 * @param beta ベータ値
 * @param depth 探索深度
 * @param passed パスされたかどうか
 * @return score_t 探索スコア
 */
score_t EndNullWindowDeep(SearchTree *tree, const score_t beta, unsigned char depth, bool passed)
{
    const score_t alpha = beta - 1;
    HashData *hashData = NULL;
    uint64_t mob, pos, flip, hashCode;
    score_t score, bestScore;
    uint8 posIdx;
    uint8 bestMove;

    // 子ノード数（スタート時点でのノード数で初期化）
    uint64_t nbChildNode = tree->nodeCount;
    // 探索コスト
    uint8 cost;

    tree->nodeCount++;
    if (depth <= 0)
    {
        return Judge(tree);
    }
    if (depth == 1)
    {
        return SolveLast1(tree->stones, alpha);
    }

    if (tree->option.useHash == 1 && depth >= tree->hashDepth)
    {
        hashData = HashTableGetData(tree->nwsTable, tree->stones, depth, &hashCode);
        if (hashData != NULL && IsHashCutNullWindow(hashData, depth, alpha, &score))
            return score;
    }

    mob = CalcMobility(tree->stones);
    if (mob == 0) // 手があるか
    {
        if (passed == 1)
        { // 2連続パスなら終了
            bestMove = NOMOVE_INDEX;
            return Judge(tree); // 勝敗判定
        }
        else
        { // パスして探索続行
            SearchPassEnd(tree);
            bestScore = -EndNullWindowDeep(tree, -alpha, depth, true);
            SearchPassEnd(tree);
            bestMove = PASS_INDEX;
        }
    }
    else
    {

        bestScore = -MAX_VALUE;
        while (mob != 0)
        {
            // 着手位置・反転位置を取得
            pos = GetLSB(mob);
            mob ^= pos;
            posIdx = PosIndexFromBit(pos);
            flip = CalcFlip(tree->stones, posIdx);

            SearchUpdateEndDeep(tree, pos, flip);
            { // 子ノードを探索
                score = -EndNullWindowDeep(tree, -alpha, depth - 1, false);
            }
            SearchRestoreEndDeep(tree, pos, flip);

            if (score > bestScore)
            {
                bestScore = score;
                bestMove = posIdx;
                if (score >= beta)
                {
                    break; //探索終了
                }
            }
        }
    } // end of if(mob == 0) else

    // 「現在のノード数」と「スタート時点でのノード数」の差分＝子ノード数
    nbChildNode = tree->nodeCount - nbChildNode;
    cost = CalcCost(nbChildNode);

    if (tree->option.useHash == 1 && depth >= tree->hashDepth)
    {
        HashTableRegist(tree->nwsTable, hashCode, tree->stones, bestMove, cost, depth, alpha, beta, bestScore);
    }
    return bestScore;
}

/**
 * @brief 終盤探索NWS
 * 
 * 参考
 * https://www.chessprogramming.org/Principal_Variation_Search
 * 
 * @param tree 探索木
 * @param beta ベータ値
 * @param depth 探索深度
 * @param passed パスされたかどうか
 * @return score_t 探索スコア
 */
score_t EndNullWindow(SearchTree *tree, const score_t beta, unsigned char depth, bool passed)
{
    SearchFuncNullWindow_t NextNullSearch;
    HashData *hashData = NULL;
    MoveList moveList;
    Move *move;
    uint64_t hashCode;
    uint8 bestMove;
    score_t score, bestScore;
    const score_t alpha = beta - 1;

    // 子ノード数（スタート時点でのノード数で初期化）
    uint64_t nbChildNode = tree->nodeCount;
    // 探索コスト
    uint8 cost;

    tree->nodeCount++;
    if (depth <= 0)
    {
        return Judge(tree);
    }
    if (depth == 1)
    {
        return SolveLast1(tree->stones, alpha);
    }

    if (tree->option.useHash == 1 && depth >= tree->hashDepth)
    {
        hashData = HashTableGetData(tree->nwsTable, tree->stones, depth, &hashCode);
        if (hashData != NULL && IsHashCutNullWindow(hashData, depth, alpha, &score))
            return score;
    }

    if (depth - 1 >= tree->orderDepth)
    {
        NextNullSearch = EndNullWindow;
    }
    else
    {
        NextNullSearch = EndNullWindowDeep;
    }

    CreateMoveList(&moveList, tree->stones);
    if (moveList.nbMoves <= 0) // 手があるか
    {
        if (passed == 1)
        { // 2連続パスなら終了
            bestMove = NOMOVE_INDEX;
            return Judge(tree); // 勝敗判定
        }
        else
        { // パスして探索続行
            SearchPassEnd(tree);
            bestScore = -NextNullSearch(tree, -alpha, depth, true);
            SearchPassEnd(tree);
            bestMove = PASS_INDEX;
        }
    }
    else
    {

        EvaluateMoveList(tree, &moveList, tree->stones, alpha, hashData);

        bestScore = -MAX_VALUE;

        for (move = NextBestMoveWithSwap(moveList.moves); move != NULL; move = NextBestMoveWithSwap(move))
        {
            SearchUpdateEnd(tree, move);
            { // 子ノードを探索
                score = -NextNullSearch(tree, -alpha, depth - 1, false);
            }
            SearchRestoreEnd(tree, move);

            if (score > bestScore)
            {
                bestScore = score;
                bestMove = move->posIdx;
                if (score >= beta)
                {
                    break; //探索終了
                }
            }
        }
    } // end of if(moveList.nbMoves > 0)

    // 「現在のノード数」と「スタート時点でのノード数」の差分＝子ノード数
    nbChildNode = tree->nodeCount - nbChildNode;
    cost = CalcCost(nbChildNode);

    if (tree->option.useHash == 1 && depth >= tree->hashDepth)
    {
        HashTableRegist(tree->nwsTable, hashCode, tree->stones, bestMove, cost, depth, alpha, beta, bestScore);
    }
    return bestScore;
}

/**
 * @brief 終盤のPVS探索
 * 
 * 参考
 *  https://www.chessprogramming.org/Principal_Variation_Search
 * 
 * @param tree 探索木用データ
 * @param in_alpha アルファ値
 * @param in_beta ベータ値
 * @param depth 残りの探索深度
 * @param passed パスされたかどうか
 * @return score_t 最善手のスコア
 */

score_t EndPVS(SearchTree *tree, const score_t in_alpha, const score_t in_beta, const unsigned char depth, const bool passed)
{
    SearchFunc_t NextSearch;
    HashData *hashData = NULL;
    MoveList moveList;
    Move *move;
    uint64_t hashCode;
    uint8 bestMove;
    score_t score, alpha, beta;
    score_t bestScore;

    // 子ノード数（スタート時点でのノード数で初期化）
    uint64_t nbChildNode = tree->nodeCount;
    // 探索コスト
    uint8 cost;

    tree->nodeCount++;
    if (depth <= 0)
    {
        return Judge(tree);
    }
    if (depth == 1)
    {
        return SolveLast1(tree->stones, in_alpha);
    }

    alpha = in_alpha;
    beta = in_beta;
    if (tree->option.usePvHash == 1 && depth >= tree->hashDepth)
    { // ハッシュの記録をもとにカット/探索範囲の縮小
        hashData = HashTableGetData(tree->pvTable, tree->stones, depth, &hashCode);
        if (hashData != NULL && IsHashCut(hashData, depth, &alpha, &beta, &score))
            return score;
    }

    if (depth - 1 >= tree->pvsDepth)
    {
        NextSearch = EndPVS;
    }
    else
    {
        NextSearch = EndAlphaBeta;
    }

    CreateMoveList(&moveList, tree->stones); // 着手リストを作成

    if (moveList.nbMoves <= 0)
    { // 手があるか
        if (passed == 1)
        { // 2連続パスなら終了
            bestMove = NOMOVE_INDEX;
            return Judge(tree); // 勝敗判定
        }
        else
        { // パスして探索続行
            SearchPassEnd(tree);
            bestScore = -NextSearch(tree, -in_beta, -in_alpha, depth, true);
            SearchPassEnd(tree);
            bestMove = PASS_INDEX;
        }
    }
    else
    { // 着手可能なとき
        bestScore = -MAX_VALUE;

        // 着手の事前評価
        EvaluateMoveList(tree, &moveList, tree->stones, alpha, hashData);

        for (move = NextBestMoveWithSwap(moveList.moves); move != NULL; move = NextBestMoveWithSwap(move))
        { // すべての着手についてループ
            SearchUpdateEnd(tree, move);
            if (bestScore == -MAX_VALUE)
            {                                                               // PVが見つかっていない
                score = -NextSearch(tree, -beta, -alpha, depth - 1, false); // 通常探索
            }
            else
            {                                                           // PVが見つかっている
                score = -EndNullWindow(tree, -alpha, depth - 1, false); // 最善かどうかチェック 子ノードをNull Window探索
                if (score > alpha)                                      // 予想が外れていたら
                {
                    score = -NextSearch(tree, -beta, -alpha, depth - 1, false); // 通常のWindowで再探索
                }
            }
            SearchRestoreEnd(tree, move);

            if (score > bestScore)
            {
                bestScore = score;
                bestMove = move->posIdx;

                if (score >= beta) // 上限突破したら
                {
                    tree->nbCut++;
                    break; // 探索終了（カット）
                }
                if (bestScore > alpha) // alphaを上回る着手を発見したら
                {
                    alpha = bestScore;
                }
            }
        } // end of moves loop
    }

    // 「現在のノード数」と「スタート時点でのノード数」の差分＝子ノード数
    nbChildNode = tree->nodeCount - nbChildNode;
    cost = CalcCost(nbChildNode);

    if (tree->option.usePvHash == 1 && depth >= tree->hashDepth)
    {
        HashTableRegist(tree->pvTable, hashCode, tree->stones, bestMove, cost, depth, in_alpha, in_beta, bestScore);
    }
    return bestScore;
}

/**
 * @brief 終盤探索のルートノード処理
 * 
 * @param tree 探索木
 * @param choiceSecond 次善手を選ぶか
 * @return uint8 予測手の位置番号
 */
uint8 EndRoot(SearchTree *tree, bool choiceSecond)
{
    // 探索中の予想最善スコアマップ
    score_t latestScoreMap[64];
    ResetScoreMap(latestScoreMap);
    ResetScoreMap(tree->scoreMap);

    uint8 depth = tree->depth;
    SearchFunc_t NextSearch;
    if (depth - 1 >= tree->pvsDepth)
    {
        NextSearch = EndPVS;
    }
    else
    {
        NextSearch = EndAlphaBeta;
    }

    score_t score, alpha, beta;
    score_t bestScore;
    alpha = -MAX_VALUE;
    beta = MAX_VALUE;
    bestScore = -MAX_VALUE;

    HashData *hashData = NULL;
    uint64_t hashCode;
    if (tree->option.usePvHash)
    {
        hashData = HashTableGetData(tree->pvTable, tree->stones, depth, &hashCode);
        if (hashData != NULL)
        {
            IsHashCut(hashData, depth, &alpha, &beta, &score);
        }
    }

    MoveList moveList;
    CreateMoveList(&moveList, tree->stones);                          // 着手リストを作成
    EvaluateMoveList(tree, &moveList, tree->stones, alpha, hashData); // 着手の事前評価
    assert(moveList.nbMoves > 0);

    uint8 bestMove = NOMOVE_INDEX, secondMove = NOMOVE_INDEX;
    for (Move *move = NextBestMoveWithSwap(moveList.moves); move != NULL; move = NextBestMoveWithSwap(move))
    { // すべての着手についてループ
        SearchUpdateEnd(tree, move);
        if (bestScore == -MAX_VALUE)
        {                                                                                              // PVが見つかっていない
            latestScoreMap[move->posIdx] = score = -NextSearch(tree, -beta, -alpha, depth - 1, false); // 通常探索
        }
        else
        {                                                           // PVが見つかっている
            score = -EndNullWindow(tree, -alpha, depth - 1, false); // 最善かどうかチェック 子ノードをNull Window探索
            if (score > alpha)                                      // 予想が外れていたら
            {
                score = -NextSearch(tree, -beta, -alpha, depth - 1, false); // 通常のWindowで再探索
                latestScoreMap[move->posIdx] = score;
            }
            else
            {
                // カットされたときは最高スコアより低くマッピング
                latestScoreMap[move->posIdx] = score - 1;
            }
        }
        SearchRestoreEnd(tree, move);

        if (score > bestScore) // alphaを上回る着手を発見したら
        {
            bestScore = score;
            bestMove = move->posIdx;
            if (bestScore > alpha)
            {
                alpha = bestScore;
            }
        }

        // 探索の中断
        if (tree->killFlag)
        {
            tree->isIntrrupted = true;
            return NOMOVE_INDEX;
        }
    } // end of moves loop

    if (tree->isIntrrupted)
    {
        printf("Search Interrupted!!! 探索を中断します\n");
    }
    else
    {
        UpdateScoreMap(latestScoreMap, tree->scoreMap);
    }

    bestScore = MIN_VALUE;
    uint8 bestPos;
    for (int pos = 0; pos < 64; pos++)
    {
        if (tree->scoreMap[pos] > bestScore)
        {
            bestScore = tree->scoreMap[pos];
            bestPos = pos;
        }
    }
    tree->score = bestScore;
    tree->completeDepth = depth;

    if (choiceSecond && secondMove != NOMOVE_INDEX)
    {
        return secondMove;
    }
    return bestPos;
}