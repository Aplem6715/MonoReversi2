#include <stdio.h>
#include <assert.h>
#include "moves.h"
#include "mid.h"
#include "search.h"
#include "../bit_operation.h"

void CreateMoveList(MoveList *moveList, uint64 own, uint64 opp)
{
    Move *prev = moveList->moves;
    Move *move = moveList->moves + 1;
    uint8 posIdx;
    uint64 pos, rev;
    uint64 mob = CalcMobility(own, opp);

    while (mob != 0)
    {
        // 着手位置・反転位置を取得
        pos = GetLSB(mob);
        posIdx = CalcPosIndex(pos);
        mob ^= pos;
        rev = CalcFlipOptimized(own, opp, posIdx);

        move->flip = rev;
        move->posIdx = posIdx;
        prev = prev->next = move;
        move++;
    }
    // 12, 21, 22, 30, 37, 45, 47, 55, 62
    prev->next = NULL;
    moveList->nbMoves = (uint8)(move - moveList->moves - 1);

    assert(moveList->nbMoves == CountBits(CalcMobility(own, opp)));
}

void EvaluateMove(SearchTree *tree, Move *move, uint64 own, uint64 opp, const HashData *hashData)
{
    if (move->flip == opp)
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
        uint64 posBit = CalcPosBit(move->posIdx);
        uint64 next_mob = CalcMobility(opp ^ move->flip, own ^ move->flip ^ posBit);
        uint64 next_own = opp ^ move->flip;
        uint64 next_opp = own ^ move->flip ^ posBit;
        float score;
        uint16 score16;

        // 着手位置でスコア付け(8~0bit)
        move->score = VALUE_TABLE[move->posIdx];

        // 一手読みのスコア付け（24~8bit目)
        // 着手して相手のターンに進める
        UpdateEval(tree->eval, move->posIdx, move->flip);
        score = EvalNNet(tree->eval);
        score16 = (uint16)(SCORE_MAX - score);
        // 相手のスコアを±反転してスコア加算
        move->score += (score16 * (1 << 8));
        UndoEval(tree->eval, move->posIdx, move->flip);

        // 相手の着手位置が多いとマイナス，少ないとプラス(14~8bit目)
        move->score += (MAX_MOVES + 4 /*角分*/ - (CountBits(next_mob) + CountBits(next_mob & 0x8100000000000081))) * (1 << 8);

        // 置換表に含まれていたらプラス(8bit目)
        if (HashContains(tree->table, next_own, next_opp))
        {
            move->score += (1 << 8);
        }
    }
}

void EvaluateMoveList(SearchTree *tree, MoveList *movelist, uint64 own, uint64 opp, const HashData *hashData)
{
    Move *move;
    for (move = movelist->moves->next; move != NULL; move = move->next)
    {
        EvaluateMove(tree, move, own, opp, hashData);
    }
}

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

void SortMoveList(MoveList *moveList)
{
    Move *move;
    for (move = NextBestMoveWithSwap(moveList->moves); move != NULL; move = NextBestMoveWithSwap(move))
        ;
}
