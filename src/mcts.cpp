#include <math.h>
#include "mcts.h"
#include "bit_operation.h"

void Node::Init(uint64 own, uint64 opp)
{
    this->own = own;
    this->opp = opp;
    this->n = 0;
    this->w = 0;
}

void Node::Expand()
{
    uint64 mobility = CalcMobility(own, opp);
    uint64 pos = 0;
    uint64 flips = 0;

    // パスの場合
    if (mobility == 0)
    {
        // 手番を反転させた子ノードを追加（手番以外変化なしの子ノード）
        childs.push_back(pool->GetNewNode(opp, own));
        return;
    }

    while (mobility != 0)
    {
        pos = GetLSB(mobility);
        mobility ^= pos;

        flips = CalcFlip(own, opp, pos);
        childs.push_back(pool->GetNewNode(opp ^ flips, own ^ flips ^ pos));
    }
}

double Node::Ucb(unsigned int parentN)
{
    return w / n + costWeight * policyProb * sqrt(parentN) / (1 + n);
}

double Node::Next()
{
    double value = 0;
    n++;

    if (finished)
    {
        if (CountBits(own) > CountBits(opp))
        {
            value = 1;
        }
        else
        {
            value = 0;
        }
        w += value;
        return value;
    }

    // 葉ノードなら
    if (this->childs.empty())
    {
        // 訪問回数が一定以上で拡張
        value = Evaluate();
        w += value;
        // 一定以下なら現状を評価して返す

        if (this->n > expand_thresh)
        {
            Expand();
        }
        return value;
    }
    else
    {
        // 最大スコアの子ノードを探索
        double score;
        double maxScore = -1000000;
        Node *bestNode;
        for (Node *child : childs)
        {
            // 相手の手番のスコアは反転
            score = -child->Ucb(n);
            if (score > maxScore)
            {
                bestNode = child;
                maxScore = score;
            }
        }

        value = bestNode->Next();
        w += value;
        return value;
    }
}

double Node::Evaluate()
{
    return 1.0;
}