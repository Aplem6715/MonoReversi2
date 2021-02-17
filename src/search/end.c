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

#include "end.h"
#include "search.h"
#include "hash.h"
#include "moves.h"
#include "../ai/eval.h"
#include "../bit_operation.h"
#include "../const.h"
#include <assert.h>

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
 * @brief 終盤探索αβ法
 * 
 * @param tree 探索木
 * @param alpha アルファ値
 * @param beta ベータ値
 * @param depth 探索深度
 * @param passed パスされたか？
 * @return score_t 探索結果スコア
 */
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
            hashData = HashTableGetData(tree->pvTable, tree->stones, depth, &hashCode);
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

    if (tree->useHash == 1)
    {
        HashTableRegist(tree->pvTable, hashCode, tree->stones, bestMove, depth, alpha, beta, maxScore);
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

    if (tree->useHash == 1 && hashData != NULL)
    {
        HashTableRegist(tree->pvTable, hashCode, tree->stones, bestMove, depth, alpha, beta, maxScore);
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
score_t EndNullWindowDeep(SearchTree *tree, const score_t beta, unsigned char depth, unsigned char passed)
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
        return Judge(tree);
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
            maxScore = -EndNullWindowDeep(tree, -alpha, depth, true);
            SearchPassEnd(tree);
            bestMove = PASS_INDEX;
        }
    }
    else
    {
        if (tree->useHash == 1 && depth >= tree->hashDepth)
        {
            hashData = HashTableGetData(tree->nwsTable, tree->stones, depth, &hashCode);
            if (hashData != NULL && IsHashCutNullWindow(hashData, depth, alpha, &score))
                return score;
        }

        maxScore = -MAX_VALUE;
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
        HashTableRegist(tree->nwsTable, hashCode, tree->stones, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
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
score_t EndNullWindow(SearchTree *tree, const score_t beta, unsigned char depth, unsigned char passed)
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
        return Judge(tree);
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
            maxScore = -NextNullSearch(tree, -alpha, depth, true);
            SearchPassEnd(tree);
            bestMove = PASS_INDEX;
        }
    }
    else
    {
        if (tree->useHash == 1 && depth >= tree->hashDepth)
        {
            hashData = HashTableGetData(tree->nwsTable, tree->stones, depth, &hashCode);
            if (hashData != NULL && IsHashCutNullWindow(hashData, depth, alpha, &score))
                return score;
        }

        EvaluateMoveList(tree, &moveList, tree->stones, hashData);

        maxScore = -MAX_VALUE;

        for (move = NextBestMoveWithSwap(moveList.moves); move != NULL; move = NextBestMoveWithSwap(move))
        {
            SearchUpdateEnd(tree, move);
            { // 子ノードを探索
                score = -NextNullSearch(tree, -alpha, depth - 1, false);
            }
            SearchRestoreEnd(tree, move);

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
        HashTableRegist(tree->nwsTable, hashCode, tree->stones, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
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

score_t EndPVS(SearchTree *tree, const score_t in_alpha, const score_t in_beta, const unsigned char depth, const unsigned char passed)
{
    SearchFunc_t NextSearch;
    HashData *hashData = NULL;
    MoveList moveList;
    Move *move;
    uint64_t hashCode;
    uint8 bestMove;
    score_t score, alpha, beta;
    score_t bestScore;

    tree->nodeCount++;
    if (depth <= 0)
    {
        return Judge(tree);
    }

    alpha = in_alpha;
    beta = in_beta;
    if (tree->useHash == 1 && depth >= tree->hashDepth)
    { // ハッシュの記録をもとにカット/探索範囲の縮小
        hashData = HashTableGetData(tree->pvTable, tree->stones, depth, &hashCode);
        // PVノードはカットしない(性能も殆ど変わらなかった)
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

        EvaluateMoveList(tree, &moveList, tree->stones, hashData); // 着手の事前評価

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

    if (tree->useHash == 1)
    {
        HashTableRegist(tree->pvTable, hashCode, tree->stones, bestMove, depth, in_alpha, in_beta, bestScore);
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
    SearchFunc_t NextSearch;
    score_t score, alpha, beta;
    uint8 bestMove = NOMOVE_INDEX, secondMove = NOMOVE_INDEX;
    bool foundPV = 0;
    uint8 depth = tree->depth;
    MoveList moveList;
    Move *move;

    if (depth - 1 >= tree->pvsDepth)
    {
        NextSearch = EndPVS;
    }
    else
    {
        NextSearch = EndAlphaBeta;
    }

    alpha = -MAX_VALUE;
    beta = MAX_VALUE;

    CreateMoveList(&moveList, tree->stones);               // 着手リストを作成
    EvaluateMoveList(tree, &moveList, tree->stones, NULL); // 着手の事前評価
    assert(moveList.nbMoves > 0);

    for (move = NextBestMoveWithSwap(moveList.moves); move != NULL; move = NextBestMoveWithSwap(move))
    { // すべての着手についてループ
        SearchUpdateEnd(tree, move);
        if (!foundPV)
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

        if (score > alpha) // alphaを上回る着手を発見したら
        {
            alpha = score;
            bestMove = move->posIdx;
            foundPV = true; // PVを発見した！
        }
    } // end of moves loop

    tree->score = alpha;

    if (choiceSecond && secondMove != NOMOVE_INDEX)
    {
        return secondMove;
    }
    return bestMove;
}