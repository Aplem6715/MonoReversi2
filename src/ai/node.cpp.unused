#include "node.h"
#include "bit_operation.h"

unsigned int Node::expand_thresh = 1;
float Node::costWeight = 1.0;

void Node::Init(NodePool *pool, uint64 own, uint64 opp, unsigned char posIdx, float transProb)
{

    this->finished = (CalcMobility(own, opp) == 0 && CalcMobility(opp, own) == 0);
    this->n = 0;
    this->w = 0;
    this->policyProb = transProb;
    this->posIdx = posIdx;
    this->own = own;
    this->opp = opp;
    this->childs.clear();
    this->pool = pool;
}

float Node::Expand()
{
    uint64 mobility = CalcMobility(own, opp);
    uint64 pos = 0;
    uint64 flips = 0;

    //vec_t result = net.predict;
    //result = {p, v};
    float probs[64] = {0};
    float v = 1.0;

    // パスの場合
    if (mobility == 0)
    {
        // 手番を反転させた子ノードを追加（手番以外変化なしの子ノード）
        childs.push_back(pool->GetNewNode(opp, own, 1, 0));
        return v;
    }

    while (mobility != 0)
    {
        pos = GetLSB(mobility);
        mobility ^= pos;
        unsigned char posIdx = CalcPosIndex(pos);

        flips = CalcFlip(own, opp, pos);
        childs.push_back(pool->GetNewNode(opp ^ flips, own ^ flips ^ pos, probs[posIdx], posIdx));
    }
    return v;
}

float Node::Ucb(unsigned int parentN)
{
    if (n == 0)
    {
        return 0 + costWeight * policyProb * (float)sqrt(parentN) / (1 + n);
    }
    return -w / n + costWeight * policyProb * (float)sqrt(parentN) / (1 + n);
}

float Node::Next()
{
    float value = 0;

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
        // value = Evaluate();
        n++;
        // 一定以下なら現状を評価して返す

        if (this->n >= expand_thresh)
        {
            value = Expand();
        }
        w += value;
        return value;
    }
    else
    {
        // 最大スコアの子ノードを探索
        float score;
        float maxScore = -1000000;
        Node *bestNode;
        unsigned int childNSum = 0;
        for (Node *child : childs)
        {
            childNSum += child->GetNumVisited();
        }

        for (Node *child : childs)
        {
            score = child->Ucb(childNSum);
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

/*
float Node::Evaluate()
{
    return 1.0;
}
*/

unsigned char Node::SelectMove(bool isDeterministic)
{
    if (isDeterministic)
    {
        int n;
        int nMax = -1;
        unsigned char bestMove = 0;
        for (Node *child : childs)
        {
            n = child->GetNumVisited();
            if (n > nMax)
            {
                nMax = n;
                bestMove = child->GetPos();
            }
        }
        return bestMove;
    }
    else
    {
        uint64 childNSum = 0;
        for (Node *child : childs)
        {
            childNSum += child->GetNumVisited();
        }
        float rndSlider = rand() / (float)RAND_MAX;
        float accum = 0;
        for (Node *child : childs)
        {
            accum += child->GetNumVisited() / (float)childNSum;
            if (accum >= rndSlider)
            {
                return child->GetPos();
            }
        }
        printf(">>>>>>>>>>>> Unreachable Code? <<<<<<<<<<<<<\n");
        return childs.back()->GetPos();
    }
}

NodePool::NodePool(int initSize, int additionSize)
{
    this->additionSize = additionSize;
    for (int i = 0; i < initSize; i++)
    {
        pool.push_back(new Node());
    }
}

NodePool::~NodePool()
{
    Node *node;
    while (!pool.empty())
    {
        node = pool.back();
        pool.pop_back();
        delete node;
    }
}

Node *NodePool::GetNewNode(uint64 own, uint64 opp, float transProb, unsigned char posIdx)
{
    if (pool.empty())
    {
        for (int i = 0; i < additionSize; i++)
        {
            pool.push_back(new Node());
        }
    }
    Node *node = pool.back();
    pool.pop_back();

    node->Init(this, own, opp, posIdx, transProb);
    return node;
}

void NodePool::RemoveNode(Node *node)
{
    pool.push_back(node);
}
