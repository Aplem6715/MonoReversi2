#include <math.h>
#include "mcts.h"
#include "bit_operation.h"

int Node::expand_thresh = 1;

void Node::Init(NodePool *pool, uint64 own, uint64 opp, double transProb)
{
    this->finished = (CalcMobility(own, opp) == 0 && CalcMobility(opp, own));
    this->n = 0;
    this->w = 0;
    this->policyProb = transProb;
    this->own = own;
    this->opp = opp;
    this->childs.clear();
    this->pool = pool;
}

double Node::Expand()
{
    uint64 mobility = CalcMobility(own, opp);
    uint64 pos = 0;
    uint64 flips = 0;

    // パスの場合
    if (mobility == 0)
    {
        // 手番を反転させた子ノードを追加（手番以外変化なしの子ノード）
        childs.push_back(pool->GetNewNode(opp, own, 1));
        return;
    }

    //vec_t result = net.predict;
    //result = {p, v};
    double probs[64] = {0};
    double v = 1.0;

    while (mobility != 0)
    {
        pos = GetLSB(mobility);
        mobility ^= pos;
        unsigned char posIdx = CalcPosIndex(pos);

        flips = CalcFlip(own, opp, pos);
        childs.push_back(pool->GetNewNode(opp ^ flips, own ^ flips ^ pos, probs[posIdx]));
    }
}

double Node::Ucb(unsigned int parentN)
{
    return w / n + costWeight * policyProb * sqrt(parentN) / (1 + n);
}

double Node::Next()
{
    double value = 0;

    // ゲーム終端なら勝敗を評価として返す
    if (finished)
    {
        // 勝ちは0, 負けは-1
        if (CountBits(own) > CountBits(opp))
        {
            value = 0;
        }
        else
        {
            value = -1;
        }
        w += value;
        n++;
        return value;
    }

    // 葉ノードなら
    if (this->childs.empty())
    {
        // 訪問回数が一定以上で拡張
        value = Evaluate();
        w += value;
        n++;
        // 一定以下なら現状を評価して返す

        if (this->n >= expand_thresh)
        {
            value = Expand();
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
        n++;
        return value;
    }
}

double Node::Evaluate()
{
    return 1.0;
}

NodePool::NodePool(int initSize)
{
    pool.reserve(initSize);
}

Node *NodePool::GetNewNode(uint64 own, uint64 opp, double transProb)
{
    Node *node = pool.back();
    pool.pop_back();

    node->Init(this, own, opp, transProb);
    return node;
}

void NodePool::RemoveNode(Node *node)
{
    pool.push_back(node);
}