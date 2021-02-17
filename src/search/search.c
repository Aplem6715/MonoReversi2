/**
 * @file search.c
 * @author Daichi Sato
 * @brief 探索AIのメイン実装部
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 * 残り空きマス数から中盤探索・終盤探索を判断し実行する
 * その他，探索中の更新処理なども行う。
 * 
 */

#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "search.h"
#include "mid.h"
#include "end.h"
#include "../ai/nnet.h"
#include "../bit_operation.h"

static const uint8 FIRST_MOVES_INDEX[] = {19, 26, 37, 44};

/**
 * @brief 探索木の生成
 * 
 * @param tree 生成した探索木
 * @param midDepth 中盤探索深度
 * @param endDepth 終盤探索深度
 * @param midPvsDepth 中盤探索PVS-αβ切り替え深度
 * @param endPvsDepth 終盤探索PVS-αβ切り替え深度
 * @param useHash ハッシュ表を使うか
 * @param useMPC Multi Prob Cutを使うか
 * @param nestMPC MPCの浅い探索中にさらにMPCを許可するか
 */
void InitTree(SearchTree *tree, unsigned char midDepth, unsigned char endDepth, unsigned char midPvsDepth, unsigned char endPvsDepth, bool useHash, bool usePvHash, bool useMPC, bool nestMPC)
{
    tree->midDepth = midDepth;
    tree->endDepth = endDepth;
    tree->midPvsDepth = midPvsDepth;
    tree->endPvsDepth = endPvsDepth;
    tree->useHash = useHash;
    tree->usePvHash = usePvHash;
    tree->useMPC = useMPC;
    tree->enableMpcNest = nestMPC;

    tree->useIDDS = 1;

    EvalInit(tree->eval);

    if (useHash)
    {
        tree->nwsTable = (HashTable *)malloc(sizeof(HashTable));
        tree->pvTable = (HashTable *)malloc(sizeof(HashTable));
        if (tree->nwsTable == NULL || tree->pvTable == NULL)
        {
            printf("ハッシュテーブルのメモリ確保失敗\n");
            return;
        }

        HashTableInit(tree->nwsTable, NWS_TABLE_SIZE);
        HashTableInit(tree->pvTable, PV_TABLE_SIZE);
    }
}

/**
 * @brief 探索木の解放
 * 
 * @param tree 解放する探索木
 */
void DeleteTree(SearchTree *tree)
{
    EvalDelete(tree->eval);
    if (tree->useHash)
    {
        HashTableFree(tree->nwsTable);
        HashTableFree(tree->pvTable);
        free(tree->nwsTable);
        free(tree->pvTable);
    }
}

/**
 * @brief 探索深度の設定変更
 * 
 * @param tree 探索木
 * @param midDepth 中盤探索深度
 * @param endDepth 終盤探索深度
 */
void ConfigTree(SearchTree *tree, unsigned char midDepth, unsigned char endDepth)
{
    tree->midDepth = midDepth;
    tree->endDepth = endDepth;
}

/**
 * @brief 探索木をリセット
 * 
 * @param tree 探索木
 */
void ResetTree(SearchTree *tree)
{
    if (tree->useHash)
    {
        HashTableReset(tree->nwsTable);
        HashTableReset(tree->pvTable);
    }
}

/**
 * @brief 探索の初期化
 * 
 * @param tree 探索木
 * @param own 自分の石情報
 * @param opp 相手の石情報
 */
void SearchSetup(SearchTree *tree, uint64_t own, uint64_t opp)
{
    //ResetTree(tree);
    // 評価パターンの初期化
    EvalReload(tree->eval, own, opp, OWN);

    tree->nbEmpty = CountBits(~(own | opp));
    tree->nodeCount = 0;
    tree->nbCut = 0;
    tree->nbMpcNested = 0;

    tree->stones->own = own;
    tree->stones->opp = opp;
}

/**
 * @brief 中盤探索でのパス・パス戻し処理
 * 
 * @param tree 探索木
 */
void SearchPassMid(SearchTree *tree)
{
    EvalUpdatePass(tree->eval);
    StonesSwap(tree->stones);
}

/**
 * @brief 中盤探索での更新処理
 * 
 * @param tree 探索木
 * @param move 着手情報
 */
void SearchUpdateMid(SearchTree *tree, Move *move)
{
    uint64_t posBit = CalcPosBit(move->posIdx);
    EvalUpdate(tree->eval, move->posIdx, move->flip);
    StonesUpdate(tree->stones, posBit, move->flip);
    tree->nbEmpty--;
}

/**
 * @brief 中盤探索での盤面復元処理
 * 
 * @param tree 探索木
 * @param move 着手情報
 */
void SearchRestoreMid(SearchTree *tree, Move *move)
{
    uint64_t posBit = CalcPosBit(move->posIdx);
    EvalUndo(tree->eval, move->posIdx, move->flip);
    StonesRestore(tree->stones, posBit, move->flip);
    tree->nbEmpty++;
}

/**
 * @brief 深い深度での中盤更新処理
 * 
 * @param tree 探索木
 * @param pos bit着手位置
 * @param flip bit反転位置
 */
void SearchUpdateMidDeep(SearchTree *tree, uint64_t pos, uint64_t flip)
{
    uint8 posIdx = PosIndexFromBit(pos);
    EvalUpdate(tree->eval, posIdx, flip);
    StonesUpdate(tree->stones, pos, flip);
    tree->nbEmpty--;
}

/**
 * @brief 深い深度での中盤復元処理
 * 
 * @param tree 探索木
 * @param pos bit着手位置
 * @param flip bit反転位置
 */
void SearchRestoreMidDeep(SearchTree *tree, uint64_t pos, uint64_t flip)
{
    uint8 posIdx = PosIndexFromBit(pos);
    EvalUndo(tree->eval, posIdx, flip);
    StonesRestore(tree->stones, pos, flip);
    tree->nbEmpty++;
}

/**
 * @brief 終盤探索でのパス・パス復元処理
 * 
 * @param tree 
 */
void SearchPassEnd(SearchTree *tree)
{
    StonesSwap(tree->stones);
    EvalUpdatePass(tree->eval);
}

/**
 * @brief 終盤探索での更新処理
 * 
 * @param tree 探索木
 * @param move 着手情報
 */
void SearchUpdateEnd(SearchTree *tree, Move *move)
{
    EvalUpdate(tree->eval, move->posIdx, move->flip);
    StonesUpdate(tree->stones, CalcPosBit(move->posIdx), move->flip);
    tree->nbEmpty--;
}

/**
 * @brief 終盤探索での復元処理
 * 
 * @param tree 探索木
 * @param move 着手情報
 */
void SearchRestoreEnd(SearchTree *tree, Move *move)
{
    EvalUndo(tree->eval, move->posIdx, move->flip);
    StonesRestore(tree->stones, CalcPosBit(move->posIdx), move->flip);
    tree->nbEmpty++;
}

/**
 * @brief 深い深度での終盤更新処理
 * 
 * @param tree 探索木
 * @param pos bit着手位置
 * @param flip bit反転位置
 */
void SearchUpdateEndDeep(SearchTree *tree, uint64_t pos, uint64_t flip)
{
    uint8 posIdx = PosIndexFromBit(pos);
    StonesUpdate(tree->stones, pos, flip);
    tree->nbEmpty--;
}

/**
 * @brief 深い深度での終盤探索復元処理
 * 
 * @param tree 探索木
 * @param pos bit着手位置
 * @param flip bit反転位置
 */
void SearchRestoreEndDeep(SearchTree *tree, uint64_t pos, uint64_t flip)
{
    uint8 posIdx = PosIndexFromBit(pos);
    StonesRestore(tree->stones, pos, flip);
    tree->nbEmpty++;
}

/**
 * @brief 予想最善手の探索（AIのメイン処理）
 * 
 * @param tree 探索木
 * @param own 自身の石配置
 * @param opp 相手の石配置
 * @param choiceSecond 次善手を選ぶかどうか
 * @return uint8 予想最善手の位置番号
 */
uint8 Search(SearchTree *tree, uint64_t own, uint64_t opp, bool choiceSecond)
{
    uint8 pos = NOMOVE_INDEX;
    uint8 nbEmpty = CountBits(~(own | opp));

    clock_t start, finish;
    start = clock();

    SearchSetup(tree, own, opp);

    if (tree->nbEmpty == 60)
    {
        pos = FIRST_MOVES_INDEX[rand() % 4];
    }
    else if (nbEmpty <= tree->endDepth)
    {
        if (!tree->isEndSearch)
        {
            tree->isEndSearch = true;
            // 中盤 ⇔ 終盤切り替え時，スコアが切り替わるので置換表内のスコアをリセット
            if (tree->usePvHash)
                HashTableResetScoreWindows(tree->pvTable);
            if (tree->useHash)
                HashTableResetScoreWindows(tree->nwsTable);
        }
        tree->depth = nbEmpty;
        tree->pvsDepth = tree->endPvsDepth;
        tree->orderDepth = tree->pvsDepth;
        tree->hashDepth = tree->pvsDepth - 1;
        pos = EndRoot(tree, choiceSecond);
    }
    else
    {
        tree->isEndSearch = 0;
        tree->depth = tree->midDepth;
        tree->pvsDepth = tree->midPvsDepth;
        tree->orderDepth = tree->pvsDepth;
        tree->hashDepth = tree->pvsDepth - 1;
        pos = MidRoot(tree, choiceSecond);
    }

    finish = clock();
    tree->usedTime = (finish - start) / (double)CLOCKS_PER_SEC;

    sprintf_s(tree->msg, sizeof(tree->msg),
              "思考時間：%.2f[s]  探索ノード数：%zu[Node]  探索速度：%.1f[Node/s]  推定CPUスコア：%.1f",
              tree->usedTime,
              tree->nodeCount,
              tree->nodeCount / tree->usedTime,
              tree->score / (float)(STONE_VALUE));

    assert(tree->nbMpcNested == 0);
    return pos;
}
