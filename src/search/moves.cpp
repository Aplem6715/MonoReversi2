#include <stdio.h>
#include <assert.h>
#include "moves.h"
#include "mid.h"
#include "search.h"
#include "../bit_operation.h"

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
        posIdx = CalcPosIndex(pos);
        mob ^= pos;
        rev = CalcFlipOptimized(stones, posIdx);

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

void EvaluateMove(SearchTree *tree, Move *move, Stones *stones, const HashData *hashData)
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
        move->score = (uint32_t)VALUE_TABLE[move->posIdx];

        // 一手読みのスコア付け（24~8bit目)
        // 着手して相手のターンに進める
        UpdateEval(tree->eval, move->posIdx, move->flip);
        score = EvalNNet(tree->eval);
        mScore = (uint16_t)((SCORE_MAX - score) / STONE_VALUE);
        assert(SCORE_MAX - score >= 0);

        // 相手のスコアを±反転してスコア加算(精度は1石単位で)
        move->score += (mScore * (1 << 8));
        UndoEval(tree->eval, move->posIdx, move->flip);

        // 相手の着手位置が多いとマイナス，少ないとプラス(14~8bit目)
        move->score += (MAX_MOVES + 4 /*角分*/ - (CountBits(next_mob) + CountBits(next_mob & 0x8100000000000081))) * (1 << 8);

        // 置換表に含まれていたらプラス(8bit目)
        if (hashData != NULL && HashContains(tree->table, nextStones))
        {
            move->score += (1 << 8);
        }
    }
}

void EvaluateMoveList(SearchTree *tree, MoveList *movelist, Stones *stones, const HashData *hashData)
{
    Move *move;
    for (move = movelist->moves->next; move != NULL; move = move->next)
    {
        EvaluateMove(tree, move, tree->stones, hashData);
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
