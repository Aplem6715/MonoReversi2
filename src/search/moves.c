/**
 * @file moves.c
 * @author Daichi Sato
 * @brief 探索手リスト・並び替えの実装
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 * 深さ優先探索において，スコアの高いてから先に探索したほうが
 * 探索効率が良くなり，高速になることが知られている。
 * 探索手リストは，探索中の盤面内での合法手一覧を保持し，
 * それぞれの手についてスコア付けを行いソートしながら探索する。
 * 
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "moves.h"
#include "mid.h"
#include "hash.h"
#include "search.h"

#include "../const.h"
#include "../stones.h"
#include "../ai/eval.h"
#include "../bit_operation.h"

/**
 * @brief 盤面石情報から着手リストMoveListを作成
 * 
 * @param moveList 作成する着手リストへの参照
 * @param stones 盤面石情報
 */
void CreateMoveList(MoveList *moveList, Stones *stones)
{
    Move *prev = moveList->moves;
    Move *move = moveList->moves + 1;
    uint8 posIdx;
    uint64_t pos, rev;
    uint64_t mob = CalcMobility(stones);

    while (mob != 0)
    {
        // 着手位置・反転位置を取得
        pos = GetLSB(mob);
        posIdx = PosIndexFromBit(pos);
        mob ^= pos;
        rev = CalcFlip(stones, posIdx);

        move->flip = rev;
        move->posIdx = posIdx;
        move->score = 0;
        prev = prev->next = move;
        move++;
    }
    // 12, 21, 22, 30, 37, 45, 47, 55, 62
    prev->next = NULL;
    moveList->nbMoves = (uint8)(move - moveList->moves - 1);

    assert(moveList->nbMoves == CountBits(CalcMobility(stones)));
}

/**
 * @brief 着手の評価をする
 * 
 * @param tree 探索木
 * @param move 着手オブジェクト
 * @param stones 盤面石情報
 * @param alpha 浅い探索のアルファ値
 * @param hashData 盤面に対応するハッシュデータ
 */
void EvaluateMove(SearchTree *tree, Move *move, Stones *stones, score_t alpha, const HashData *hashData)
{
    if (move->flip == stones->opp)
    {
        // 完全勝利で最高得点
        move->score = (1 << 31);
    }
    else if (hashData && move->posIdx == hashData->bestMove)
    {
        // ハッシュで最善手記録があれば高得点
        move->score = (1 << 30);
    }
    else if (hashData && move->posIdx == hashData->secondMove)
    {
        // ハッシュで次善手記録があれば高得点
        move->score = (1 << 29);
    }
    else
    {

        uint64_t posBit = CalcPosBit(move->posIdx);
        Stones nextStones[1];
        nextStones->own = stones->opp ^ move->flip;
        nextStones->opp = stones->own ^ move->flip ^ posBit;

        uint64_t next_mob = CalcMobility(nextStones);

        score_t score;
        uint16_t mScore;

        // 着手位置でスコア付け(8~0bit)
        move->score = (uint8)VALUE_TABLE[move->posIdx];

        // 一手読みのスコア付け（24~8bit目)
        // 着手して相手のターンに進める
        /*
        EvalUpdate(tree->eval, move->posIdx, move->flip);
        score = Evaluate(tree->eval, tree->nbEmpty - 1);
        mScore = (uint16_t)(10.0 * (SCORE_MAX - score) / STONE_VALUE);
        assert(SCORE_MAX - score >= 0);

        // 相手のスコアを±反転してスコア加算(精度は0.1石単位で)
        move->score += (mScore * (1 << 8));
        EvalUndo(tree->eval, move->posIdx, move->flip);
        */
        //   浅い探索スコアで評価
        SearchUpdateMid(tree, move);
        {
            score = -MidAlphaBetaDeep(tree, SCORE_MIN, -alpha, 0, false);
        }
        SearchRestoreMid(tree, move);

        assert(SCORE_MAX - score >= 0);
        mScore = (uint16_t)((SCORE_MAX + score) / STONE_VALUE);
        move->score += mScore * (1 << 8);

        // 相手の着手位置が多いとマイナス，少ないとプラス(14~8bit目)
        move->score += (MAX_MOVES + 4 /*角分*/ - (CountBits(next_mob) + CountBits(next_mob & 0x8100000000000081))) * (1 << 10);
    }
}

/**
 * @brief 着手リストのすべての着手について評価
 * 
 * @param tree 探索木
 * @param movelist 着手可能位置リスト
 * @param stones 盤面石情報
 * @param alpha 浅い探索のアルファ値
 * @param hashData 盤面に対応するハッシュデータ
 */
void EvaluateMoveList(SearchTree *tree, MoveList *movelist, Stones *stones, score_t alpha, const HashData *hashData)
{
    Move *move;
    for (move = movelist->moves->next; move != NULL; move = move->next)
    {
        EvaluateMove(tree, move, tree->stones, alpha, hashData);
    }
}

/**
 * @brief prevの次に評価の高いMoveを入れ替えつつ取得
 * 
 * @param prev 検索の起点となる着手
 * @return Move* prevの次に評価の高い着手
 */
Move *NextBestMoveWithSwap(Move *prev)
{
    if (prev->next)
    {
        Move *move, *best;
        best = prev;
        for (move = best->next; move->next != NULL; move = move->next)
        {
            if (move->next->score > best->next->score)
            {
                best = move;
            }
        }
        if (best != prev)
        {
            // 最大スコアの手を
            move = best->next;
            // 切り離して
            best->next = best->next->next;
            // 次の手として挿入
            move->next = prev->next;
            prev->next = move;
        }
    }
    return prev->next;
}

/**
 * @brief 着手リスト全体をソート
 * 
 * @param moveList ソートする着手リスト
 */
void SortMoveList(MoveList *moveList)
{
    Move *move;
    for (move = NextBestMoveWithSwap(moveList->moves); move != NULL; move = NextBestMoveWithSwap(move))
        ;
}
