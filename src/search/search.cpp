
#include "search.h"
#include "../bit_operation.h"
#include "hash.h"
#include "moves.h"
#include "../learning/nnet.h"
#include <assert.h>

#include "../learning/learn_eval.h"

void InitTree(SearchTree *tree, unsigned char midDepth, unsigned char endDepth, unsigned char orderDepth, unsigned char useHash, unsigned char hashDepth)
{
    tree->midDepth = midDepth;
    tree->endDepth = endDepth;
    tree->orderDepth = orderDepth;
    tree->useHash = useHash;
    tree->hashDepth = hashDepth;

    InitEval(tree->eval);

    if (useHash)
    {
        tree->table = (HashTable *)malloc(sizeof(HashTable));
        if (tree->table == NULL)
        {
            printf("ハッシュテーブルのメモリ確保失敗\n");
            return;
        }

        InitHashTable(tree->table);
    }
}

void DeleteTree(SearchTree *tree)
{
    DeleteEval(tree->eval);
    if (tree->useHash)
    {
        FreeHashTable(tree->table);
        free(tree->table);
    }
#ifdef USE_NN
    free(tree->eval->net);
#elif USE_REGRESSION
    free(tree->eval->regr);
#endif
}

void ConfigTree(SearchTree *tree, unsigned char depth)
{
    tree->depth = depth;
}

void ResetTree(SearchTree *tree)
{
    if (tree->useHash)
        ResetHashTable(tree->table);
}

inline float Judge(const SearchTree *tree, const uint64 own, const uint64 opp)
{
    const uint8 nbOwn = CountBits(own);
    const uint8 nbOpp = 64 - tree->eval->nbEmpty - nbOwn;
    return (float)(nbOwn - nbOpp);
}

float WinJudge(const uint64 own, const uint64 opp)
{
    return (CountBits(own) - CountBits(opp)) * 10000.0f;
}

uint64 Search(SearchTree *tree, uint64 own, uint64 opp)
{
    float score, maxScore = -Const::MAX_VALUE;
    uint64 bestPos, pos, mob, rev;
    uint8 posIdx;
    SearchFunc_t SearchFunc;

    std::chrono::system_clock::time_point start, end;
    start = std::chrono::system_clock::now();
    tree->nodeCount = 0;

    // 評価パターンの初期化
    ReloadEval(tree->eval, own, opp, OWN);

    if (tree->eval->nbEmpty < tree->endDepth)
    {
        tree->depth = tree->eval->nbEmpty;
        // オーダリングが不要な探索では探索関数を変える
        if (tree->depth > tree->orderDepth)
        {
            SearchFunc = AlphaBetaEnd;
        }
        else
        {
            SearchFunc = AlphaBetaEndDeep;
        }
    }
    else
    {
        tree->depth = tree->midDepth;
        // オーダリングが不要な探索では探索関数を変える
        if (tree->depth > tree->orderDepth)
        {
            SearchFunc = AlphaBeta;
        }
        else
        {
            SearchFunc = AlphaBetaDeep;
        }
    }

    mob = CalcMobility(own, opp);
    while (mob != 0)
    {
        pos = GetLSB(mob);
        posIdx = CalcPosIndex(pos);
        mob ^= pos;
        rev = CalcFlip(own, opp, pos);

        UpdateEval(tree->eval, posIdx, rev);
        {
            score = -SearchFunc(tree, opp ^ rev, own ^ rev ^ pos, -Const::MAX_VALUE, Const::MAX_VALUE, tree->depth, false);
        }
        UndoEval(tree->eval, posIdx, rev);

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

float AlphaBetaDeep(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed)
{

    assert(depth <= tree->orderDepth);

    uint64 mob, pos, rev, hashCode;
    uint8 posIdx;
    uint8 bestMove;
    HashData *hashData = NULL;
    float score, maxScore, lower;

    tree->nodeCount++;
    if (depth <= 0)
    {
        //return EvalPosTable(own, opp);
        //return EvalTinyDnn(tree, tree->nbEmpty);
        return EvalNNet(tree->eval);
    }

    if (tree->useHash == 1 && depth >= tree->hashDepth)
    {
        hashData = GetHashData(tree->table, own, opp, depth, &hashCode);
        if (hashData != NULL && CutWithHash(hashData, &alpha, &beta, &score))
            return score;
    }

    mob = CalcMobility(own, opp);

    // 手があるか
    if (mob == 0)
    {
        // 2連続パスなら終了
        if (passed == 1)
        {
            bestMove = NOMOVE_INDEX;
            // 勝敗判定
            return WinJudge(own, opp);
        }
        else
        {
            // 手番を入れ替えて探索続行
            bestMove = PASS_INDEX;
            UpdateEvalPass(tree->eval);
            maxScore = -AlphaBetaDeep(tree, opp, own, -beta, -alpha, depth, true);
            UpdateEvalPass(tree->eval);
        }
    }
    else
    {
        maxScore = -Const::MAX_VALUE;
        lower = alpha;
        // 打つ手がある時
        while (mob != 0)
        {
            // 着手位置・反転位置を取得
            pos = GetLSB(mob);
            posIdx = CalcPosIndex(pos);
            mob ^= pos;
            rev = CalcFlip(own, opp, pos);

            UpdateEval(tree->eval, posIdx, rev);
            {
                // 子ノードを探索
                score = -AlphaBetaDeep(tree, opp ^ rev, own ^ rev ^ pos, -beta, -lower, depth - 1, false);
            }
            UndoEval(tree->eval, posIdx, rev);

            if (score > maxScore)
            {
                maxScore = score;
                bestMove = CalcPosIndex(pos);

                // 上限突破したら
                if (score >= beta)
                {
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
        SaveHashData(tree->table, hashCode, own, opp, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}

float AlphaBeta(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed)
{
    uint8 bestMove;
    uint64 pos, hashCode;
    SearchFunc_t NextSearch;
    MoveList moveList;
    Move *move;
    HashData *hashData = NULL;
    float score, maxScore, lower;

    tree->nodeCount++;
    if (depth <= 0)
    {
        //return EvalPosTable(own, opp);
        return EvalNNet(tree->eval);
    }

    if (tree->useHash == 1 && depth >= tree->hashDepth)
    {
        hashData = GetHashData(tree->table, own, opp, depth, &hashCode);
        if (hashData != NULL && CutWithHash(hashData, &alpha, &beta, &score))
            return score;
    }

    CreateMoveList(&moveList, own, opp);

    // 手があるか
    if (moveList.nbMoves <= 0)
    {
        // 2連続パスなら終了
        if (passed == 1)
        {
            bestMove = NOMOVE_INDEX;
            // 勝敗判定
            return WinJudge(own, opp);
        }
        else
        {
            // 手番を入れ替えて探索続行
            UpdateEvalPass(tree->eval);
            maxScore = -AlphaBeta(tree, opp, own, -beta, -alpha, depth, true);
            UpdateEvalPass(tree->eval);
            bestMove = PASS_INDEX;
        }
    }
    else
    {
        if (depth >= tree->orderDepth)
        {
            SortMoveList(&moveList);
            NextSearch = AlphaBeta;
        }
        else
        {
            NextSearch = AlphaBetaDeep;
        }
        maxScore = -Const::MAX_VALUE;
        lower = alpha;
        // 打つ手がある時, 良い手から並べ替えつつループ
        for (move = moveList.moves->next; move != NULL; move = move->next)
        {
            // 着手位置・反転位置を取得
            pos = CalcPosBit(move->posIdx);

            UpdateEval(tree->eval, move->posIdx, move->flip);
            {
                // 子ノードを探索
                score = -NextSearch(tree, opp ^ move->flip, own ^ move->flip ^ pos, -beta, -lower, depth - 1, false);
            }
            UndoEval(tree->eval, move->posIdx, move->flip);

            if (score > maxScore)
            {
                maxScore = score;
                bestMove = move->posIdx;

                // 上限突破したら
                if (score >= beta)
                {
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
        SaveHashData(tree->table, hashCode, own, opp, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}

float AlphaBetaEndDeep(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed)
{

    assert(depth <= tree->orderDepth);

    uint64 mob, pos, rev, hashCode;
    uint8 posIdx;
    uint8 bestMove;
    HashData *hashData = NULL;
    float score, maxScore, lower;

    tree->nodeCount++;
    if (depth <= 0)
    {
        //return EvalPosTable(own, opp);
        //return EvalTinyDnn(tree, tree->nbEmpty);
        return Judge(tree, own, opp);
    }

    if (tree->useHash == 1 && depth >= tree->hashDepth)
    {
        hashData = GetHashData(tree->table, own, opp, depth, &hashCode);
        if (hashData != NULL && CutWithHash(hashData, &alpha, &beta, &score))
            return score;
    }

    mob = CalcMobility(own, opp);

    // 手があるか
    if (mob == 0)
    {
        // 2連続パスなら終了
        if (passed == 1)
        {
            bestMove = NOMOVE_INDEX;
            // 勝敗判定
            return Judge(tree, own, opp);
        }
        else
        {
            // 手番を入れ替えて探索続行
            bestMove = PASS_INDEX;
            UpdateEvalPass(tree->eval);
            maxScore = -AlphaBetaEndDeep(tree, opp, own, -beta, -alpha, depth, true);
            UpdateEvalPass(tree->eval);
        }
    }
    else
    {
        maxScore = -Const::MAX_VALUE;
        lower = alpha;
        // 打つ手がある時
        while (mob != 0)
        {
            // 着手位置・反転位置を取得
            pos = GetLSB(mob);
            posIdx = CalcPosIndex(pos);
            mob ^= pos;
            rev = CalcFlip(own, opp, pos);

            UpdateEval(tree->eval, posIdx, rev);
            {
                // 子ノードを探索
                score = -AlphaBetaEndDeep(tree, opp ^ rev, own ^ rev ^ pos, -beta, -lower, depth - 1, false);
            }
            UndoEval(tree->eval, posIdx, rev);

            if (score > maxScore)
            {
                maxScore = score;
                bestMove = CalcPosIndex(pos);

                // 上限突破したら
                if (score >= beta)
                {
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
        SaveHashData(tree->table, hashCode, own, opp, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}

float AlphaBetaEnd(SearchTree *tree, uint64 own, uint64 opp, float alpha, float beta, unsigned char depth, unsigned char passed)
{
    uint8 bestMove;
    uint64 pos, hashCode;
    SearchFunc_t NextSearch;
    MoveList moveList;
    Move *move;
    HashData *hashData = NULL;
    float score, maxScore, lower;

    tree->nodeCount++;
    if (depth <= 0)
    {
        //return EvalPosTable(own, opp);
        return Judge(tree, own, opp);
    }

    if (tree->useHash == 1 && depth >= tree->hashDepth)
    {
        hashData = GetHashData(tree->table, own, opp, depth, &hashCode);
        if (hashData != NULL && CutWithHash(hashData, &alpha, &beta, &score))
            return score;
    }

    CreateMoveList(&moveList, own, opp);

    // 手があるか
    if (moveList.nbMoves <= 0)
    {
        // 2連続パスなら終了
        if (passed == 1)
        {
            bestMove = NOMOVE_INDEX;
            // 勝敗判定
            return Judge(tree, own, opp);
        }
        else
        {
            // 手番を入れ替えて探索続行
            UpdateEvalPass(tree->eval);
            maxScore = -AlphaBetaEnd(tree, opp, own, -beta, -alpha, depth, true);
            UpdateEvalPass(tree->eval);
            bestMove = PASS_INDEX;
        }
    }
    else
    {
        if (depth >= tree->orderDepth)
        {
            SortMoveList(&moveList);
            NextSearch = AlphaBetaEnd;
        }
        else
        {
            NextSearch = AlphaBetaEndDeep;
        }
        maxScore = -Const::MAX_VALUE;
        lower = alpha;
        // 打つ手がある時, 良い手から並べ替えつつループ
        for (move = moveList.moves->next; move != NULL; move = move->next)
        {
            // 着手位置・反転位置を取得
            pos = CalcPosBit(move->posIdx);

            UpdateEval(tree->eval, move->posIdx, move->flip);
            {
                // 子ノードを探索
                score = -NextSearch(tree, opp ^ move->flip, own ^ move->flip ^ pos, -beta, -lower, depth - 1, false);
            }
            UndoEval(tree->eval, move->posIdx, move->flip);

            if (score > maxScore)
            {
                maxScore = score;
                bestMove = move->posIdx;

                // 上限突破したら
                if (score >= beta)
                {
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
        SaveHashData(tree->table, hashCode, own, opp, bestMove, depth, alpha, beta, maxScore);
    }
    return maxScore;
}