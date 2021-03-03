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
#include "../debug_util.h"

static const uint8 FIRST_MOVES_INDEX[] = {19, 26, 37, 44};

void ResetScoreMap(score_t scoreMap[64])
{
    for (int pos = 0; pos < 64; pos++)
    {
        scoreMap[pos] = MIN_VALUE;
    }
}

void UpdateScoreMap(score_t latest[64], score_t complete[64])
{
    for (int pos = 0; pos < 64; pos++)
    {
        complete[pos] = latest[pos];
    }
}

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
void TreeInit(SearchTree *tree)
{
    tree->option = DEFAULT_OPTION;

    tree->killFlag = false;
    if (tree->option.useIDDS)
    {
        tree->option.useTimeLimit = tree->option.useTimeLimit;
        tree->option.oneMoveTime = SEARCH_TIME_SECONDS;
    }
    else if (tree->option.useTimeLimit)
    {
        printf("反復深化が無効です。時間制限機能は無視されます。\n");
        tree->option.useTimeLimit = false;
    }

    EvalInit(tree->eval);

    if (tree->option.useHash)
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
void TreeDelete(SearchTree *tree)
{
    EvalDelete(tree->eval);
    if (tree->option.useHash)
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
 * @param oneMoveTime 一手にかける時間
 * @param useIDD 反復深化のトグル
 * @param useTimer 時間制限トグル
 * @param useMPC MPC利用トグル
 */
void TreeConfig(SearchTree *tree, unsigned char midDepth, unsigned char endDepth, int oneMoveTime, bool useIDD, bool useTimer, bool useMPC)
{
    tree->option.midDepth = midDepth;
    tree->option.endDepth = endDepth;
    tree->option.oneMoveTime = oneMoveTime;
    tree->option.useIDDS = useIDD;
    tree->option.useTimeLimit = useTimer;
    tree->option.useMPC = useMPC;

    if (!tree->option.useIDDS && useTimer)
    {
        printf("反復深化が無効です。時間制限機能は無視されます。\n");
        tree->option.useTimeLimit = false;
    }
}

void TreeConfigClone(SearchTree *tree, SearchOption newOption)
{
    tree->option = newOption;
}

void TreeConfigDepth(SearchTree *tree, unsigned char midDepth, unsigned char endDepth)
{
    tree->option.midDepth = midDepth;
    tree->option.endDepth = endDepth;
}

void TreeClone(SearchTree *src, SearchTree *dst)
{
    *(dst->stones) = *(src->stones);
    dst->option = src->option;

    dst->nbEmpty = src->nbEmpty;

    dst->depth = src->depth;
    dst->orderDepth = src->orderDepth;
    dst->hashDepth = src->hashDepth;
    dst->pvHashDepth = src->pvHashDepth;
    dst->pvsDepth = src->pvsDepth;

    dst->nbMpcNested = src->nbMpcNested;

    HashTableClone(src->nwsTable, dst->nwsTable);
    HashTableClone(src->pvTable, dst->pvTable);
    EvalClone(src->eval, dst->eval);
}

/**
 * @brief 探索木をリセット
 * 
 * @param tree 探索木
 */
void TreeReset(SearchTree *tree)
{
    if (tree->option.useHash)
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
    //TreeReset(tree);
    // 評価パターンの初期化
    EvalReload(tree->eval, own, opp, OWN);
    if (tree->option.usePvHash)
    {
        HashTableVersionUp(tree->pvTable);
    }
    if (tree->option.useHash)
    {
        HashTableVersionUp(tree->nwsTable);
    }

    tree->nbEmpty = CountBits(~(own | opp));
    tree->nodeCount = 0;
    tree->nbCut = 0;
    tree->nbMpcNested = 0;

    tree->stones->own = own;
    tree->stones->opp = opp;
}

void SearchMutexSetup(SearchTree *tree, HANDLE timerMutex)
{
    tree->timerMutex = timerMutex;
}

bool SearchIsTimeup(SearchTree *tree)
{
    bool isTimeup;
    WaitForSingleObject(tree->timerMutex, INFINITE);
    isTimeup = clock() > tree->timeLimit;
    ReleaseMutex(tree->timerMutex);
    return isTimeup;
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

uint8 SearchWithoutSetup(SearchTree *tree)
{
    uint8 pos = NOMOVE_INDEX;
    tree->isIntrrupted = false;

    clock_t start, finish;
    start = clock();
    ResetScoreMap(tree->scoreMap);

    if (tree->nbEmpty == 60)
    {
        DEBUG_PRINTF("\tSearchWithoutSetup FirstPut\n");
        pos = FIRST_MOVES_INDEX[rand() % 4];
        tree->scoreMap[pos] = 0;

        tree->completeDepth = 1;
        tree->score = 0;
        tree->nodeCount = 1;
    }
    else if (tree->nbEmpty <= tree->option.endDepth)
    {
        DEBUG_PRINTF("\tSearchWithoutSetup End\n");
        if (!tree->isEndSearch)
        {
            tree->isEndSearch = true;
            // 中盤 ⇔ 終盤切り替え時，スコアが切り替わるので置換表内のスコアをリセット
            if (tree->option.usePvHash)
                HashTableResetScoreWindows(tree->pvTable);
            if (tree->option.useHash)
                HashTableResetScoreWindows(tree->nwsTable);
        }
        tree->depth = tree->nbEmpty;
        tree->pvsDepth = tree->option.endPvsDepth;
        tree->orderDepth = tree->pvsDepth;
        tree->hashDepth = tree->pvsDepth;
        tree->pvHashDepth = tree->pvsDepth - 1;
        pos = EndRoot(tree, tree->option.choiceSecond);
    }
    else
    {
        DEBUG_PRINTF("\tSearchWithoutSetup Mid:%d\n", tree->option.midDepth);
        tree->isEndSearch = 0;
        tree->depth = tree->option.midDepth;
        tree->pvsDepth = tree->option.midPvsDepth;
        tree->orderDepth = tree->pvsDepth;
        tree->hashDepth = tree->pvsDepth;
        tree->pvHashDepth = tree->pvsDepth - 1;
        pos = MidRoot(tree, tree->option.choiceSecond);
    }

    finish = clock();
    tree->usedTime = (finish - start) / (double)CLOCKS_PER_SEC;

    sprintf_s(tree->msg, sizeof(tree->msg),
              "探索深度: %d  思考時間：%.2f[s]  探索ノード数：%zu[kNode]  探索速度：%.1f[kNode/s]  推定CPUスコア：%.1f",
              tree->completeDepth,
              tree->usedTime,
              tree->nodeCount / 1000,
              tree->nodeCount / 1000 / tree->usedTime,
              tree->score / (float)(STONE_VALUE));

    assert(tree->nbMpcNested == 0);

    tree->bestMove = pos;
    return pos;
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
uint8 SearchWithSetup(SearchTree *tree, uint64_t own, uint64_t opp, bool choiceSecond)
{
    DEBUG_PRINTF("\tSearchWithSetup\n");
    SearchSetup(tree, own, opp);
    tree->option.choiceSecond = choiceSecond;
    uint8 pos = SearchWithoutSetup(tree);

    return pos;
}
