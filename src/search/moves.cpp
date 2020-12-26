#include <stdio.h>
#include <assert.h>
#include "moves.h"
#include "eval.h"
#include "search.h"
#include "../bit_operation.h"

void CreateMoveList(MoveList *moveList, uint64 own, uint64 opp)
{
    Move *prev = moveList->moves;
    Move *move = moveList->moves + 1;
    uint64 pos, rev;
    uint64 mob = CalcMobility(own, opp);

    while (mob != 0)
    {
        // 着手位置・反転位置を取得
        pos = GetLSB(mob);
        mob ^= pos;
        rev = CalcFlip(own, opp, pos);

        move->flip = rev;
        move->posIdx = CalcPosIndex(pos);
        prev = prev->next = move;
        move++;
    }
    prev->next = NULL;
    moveList->nbMoves = (uint8)(move - moveList->moves - 1);

    assert(moveList->nbMoves == CountBits(CalcMobility(own, opp)));
}

void EvaluateMove(SearchTree *tree, Move *move, uint64 own, uint64 opp, float alpha, const HashData *hashData)
{
    if (move->flip == opp)
    {
        move->score = (1 << 30);
    }
    else if (move->posIdx == hashData->bestMove)
    {
        move->score = (1 << 29);
    }
    else if (move->posIdx == hashData->secondMove)
    {
        move->score = (1 << 28);
    }
    else
    {
        // 着手位置でスコア付け
        move->score = (int)VALUE_TABLE[move->posIdx];

        // 相手の着手位置が多いとマイナス，少ないとプラス
        uint64 posBit = CalcPosBit(move->posIdx);
        uint64 next_mob = CalcMobility(opp ^ move->flip, own ^ move->flip ^ posBit);
        move->score += (-(CountBits(next_mob) + CountBits(next_mob & 0x8100000000000081))) * (1 << 12);

        move->score += ((int)(-AlphaBeta(tree, opp ^ move->flip, own ^ move->flip ^ posBit, -Const::MAX_VALUE, -alpha, 1, 0)) * (1 << 15));
    }
}

void EvaluateMoveList(SearchTree *tree, MoveList *movelist, uint64 own, uint64 opp, float alpha, const HashData *hashData)
{
    Move *move;
    for (move = movelist->moves->next; move; move = move->next)
    {
        EvaluateMove(tree, move, own, opp, alpha, hashData);
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
    for (move = moveList->moves->next; move != NULL; move = NextBestMoveWithSwap(move))
        ;
}
