
#include "search.h"
#include "../bit_operation.h"
#include "eval.h"

void InitTree(SearchTree *tree, unsigned char depth)
{
    tree->depth = depth;
}

void DeleteTree(SearchTree *tree)
{
}

uint64 Search(SearchTree *tree, uint64 own, uint64 opp)
{
    float score, maxScore = -Const::MAX_VALUE;
    uint64 bestPos, pos, mob, rev;

    std::chrono::system_clock::time_point start, end;
    start = std::chrono::system_clock::now();
    tree->nodeCount = 0;

    mob = CalcMobility(own, opp);

    while (mob != 0)
    {
        pos = GetLSB(mob);
        mob ^= pos;
        rev = CalcFlip(own, opp, pos);
        score = -AlphaBeta(tree, opp ^ rev, own ^ rev ^ pos, -Const::MAX_VALUE, Const::MAX_VALUE, tree->depth, false);

        if (score > maxScore)
        {
            maxScore = score;
            bestPos = pos;
        }
    }

    end = std::chrono::system_clock::now();
    tree->usedTime = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0);
    tree->score = maxScore;

    return bestPos;
}

void PVS(SearchTree *tree);

float AlphaBeta(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed)
{
    uint64 mob, pos, rev;

    tree->nodeCount++;
    if (depth <= 0)
    {
        return EvalPosTable(own, opp);
    }

    mob = CalcMobility(own, opp);

    // 手があるか
    if (mob == 0)
    {
        // 2連続パスなら終了
        if (passed == 1)
        {
            if (CountBits(own) > CountBits(opp))
            {
                return WIN_VALUE;
            }
            else if (CountBits(own) < CountBits(opp))
            {
                return -WIN_VALUE;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            alpha = maxf(alpha, -AlphaBeta(tree, opp, own, -beta, -alpha, depth - 1, true));
        }
    }
    else
    {
        // 打つ手がある時
        while (mob != 0)
        {
            // 着手位置・反転位置を取得
            pos = GetLSB(mob);
            mob ^= pos;
            rev = CalcFlip(own, opp, pos);

            // 子ノードを探索
            alpha = maxf(alpha, -AlphaBeta(tree, opp ^ rev, own ^ rev ^ pos, -beta, -alpha, depth - 1, false));

            // 上限突破したら
            if (alpha >= beta)
            {
                // カット
                return alpha;
            }
        }
    }
    return alpha;
}