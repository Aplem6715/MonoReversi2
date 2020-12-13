#include "mcts.h"
#include "bit_operation.h"

void Node::Init(uint64 own, uint64 opp)
{
    this->own = own;
    this->opp = opp;
    this->n = 0;
    this->w = 0;
}

double Node::Next()
{
    // 葉ノードなら
    if (!this->childs.empty())
    {
        // 訪問回数が一定以上で拡張
        if (this->n > expand_thresh)
        {
            Expand();
        }
        else
        {
            // 一定以下なら現状を評価して返す
            return Evaluate();
        }
    }

    // 最大UCBスコアの子ノードを呼ぶ
    double maxScore = -1000000;
    Node *bestNode;
    for (Node *child : childs)
    {
        if (child->Ucb() > maxScore)
        {
            bestNode = child;
        }
    }
    return bestNode->Next();
}

void Node::Expand()
{
    uint64 mobility = CalcMobility(own, opp);
    uint64 pos = 0;
    uint64 flips = 0;
    while (mobility != 0)
    {
        pos = GetLSB(mobility);
        mobility ^= pos;

        flips = CalcFlip(own, opp, pos);
        childs.push_back(pool->GetNewNode(opp ^ flips, own ^ flips ^ pos));
    }
}
