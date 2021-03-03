/**
 * @file search_manager.c
 * @author Daichi Sato
 * @brief 探索の管理を行うマネージャー
 * @version 1.0
 * @date 2021-03-01
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 * 非同期探索が有効な場合，相手の手番中にも探索を行う。
 * 浅い探索により決定した上位ｎ手の盤面を各プロセス（Branch)に割り当て
 * 非同期探索を行う。相手が着手したとき，着手位置に該当する
 * Branchは探索を続行，それ以外のプロセスの探索を終了する。
 * 
 */
#define _CRT_SECURE_NO_WARNINGS
#include <assert.h>
#include <stdio.h>

#include "search_manager.h"
#include "../bit_operation.h"
#include "../debug_util.h"

#define PRE_SEARCH_SHALLO_DEPTH 5
#define PRE_SEARCH_TOO_DEEP_DEPTH 20

/**
 * @brief 自身の着手を適用する
 * 
 * @param stones 石情報
 * @param myPos 自身の着手位置
 * @return Stones 更新後の石情報
 */
Stones ApplyOwnPut(Stones *stones, uint8 myPos)
{
    if (myPos == NOMOVE_INDEX)
    {
        return *stones;
    }
    Stones newStones;
    uint64_t flip = CalcFlip64(stones->own, stones->opp, myPos);
    uint64_t posBit = CalcPosBit(myPos);
    newStones.own = stones->own ^ (flip | posBit);
    newStones.opp = stones->opp ^ flip;

    return newStones;
}

/**
 * @brief 相手の着手を適用する
 * 
 * @param stones 石情報
 * @param enemyPos 相手の着手位置
 * @return Stones 着手後の石情報
 */
Stones ApplyEnemyPut(Stones *stones, uint8 enemyPos)
{
    if (enemyPos == NOMOVE_INDEX)
    {
        return *stones;
    }
    Stones newStones;
    uint64_t flip = CalcFlip64(stones->opp, stones->own, enemyPos);
    uint64_t posBit = CalcPosBit(enemyPos);
    newStones.opp = stones->opp ^ (flip | posBit);
    newStones.own = stones->own ^ flip;
    return newStones;
}

/**
 * @brief スコアマップをコピーする
 * 
 * @param src オリジナル
 * @param dst コピー先
 */
void CopyScoreMap(score_t src[64], score_t dst[64])
{
    for (int i = 0; i < 64; i++)
    {
        dst[i] = src[i];
    }
}

/**
 * @brief スコアマップから最善手を計算
 * 
 * @param map スコアマップ
 * @return uint8 予測最善手
 */
uint8 BestMoveFromMap(score_t map[64])
{
    score_t best = MIN_VALUE;
    uint8 bestMove = NOMOVE_INDEX;
    for (int pos = 0; pos < 64; pos++)
    {
        if (map[pos] > best)
        {
            bestMove = pos;
            best = map[pos];
        }
    }
    return bestMove;
}

/**
 * @brief branchの初期化
 * 
 * @param branch 初期化するbranch
 * @param id ブランチID
 */
void BranchInit(BranchProcess *branch, int id)
{
    TreeInit(branch->tree, false);
    branch->processHandle = NULL;
    branch->scoreMapMutex = NULL;
    branch->enemyMove = NOMOVE_INDEX;
    branch->state = BRANCH_WAIT;
    branch->id = id;
}

/**
 * @brief ブランチの解放
 * 
 * @param branch 開放するブランチ
 */
void BranchDelete(BranchProcess *branch)
{
    TreeDelete(branch->tree);
}

/**
 * @brief 非同期で探索を開始
 * 
 * @param tree 探索する探索木
 */
void StartSearchAsync(void *tree)
{
    SearchWithoutSetup((SearchTree *)tree);
    _endthread();
}

/**
 * @brief ブランチでの探索を開始する
 * 
 * @param branch BRUNCH
 * @param beforeEnemyStones 相手が着手する前の石情報
 * @param depth 探索深度
 */
void BranchLaunch(BranchProcess *branch, Stones *beforeEnemyStones, int depth, int endDepth)
{
    Stones stones = ApplyEnemyPut(beforeEnemyStones, branch->enemyMove);
    branch->tree->killFlag = false;

    // 事前探索をするときはタイマーOFF
    TreeConfig(branch->tree, depth, endDepth, 10000, true, false, false);
    SearchSetup(branch->tree, stones.own, stones.opp);

    branch->processHandle = (HANDLE)_beginthread(StartSearchAsync, 0, branch->tree);
}

/**
 * @brief BRUNCH情報のリセット
 * 
 * @param sManager 探索マネージャー
 */
void ResetBranches(SearchManager *sManager)
{
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        sManager->branches[i].enemyMove = NOMOVE_INDEX;
        sManager->branches[i].enemyScore = MIN_VALUE;
    }
}

/**
 * @brief branchをソートしつつ挿入
 * 
 * @param sManager 探索マネージャー
 * @param pos 着手位置
 * @param score 着手のスコア
 * @param idx 挿入インデックス
 */
void InsertBranch(SearchManager *sManager, uint8 pos, score_t score, int idx)
{
    BranchProcess *betterBranch;
    BranchProcess *currentBranch;
    // 終端からidxまでをシフトして，idx位置のbranchを開ける
    for (int i = sManager->numMaxBranches - 1; i > idx; i--)
    {
        betterBranch = &sManager->branches[i - 1];
        currentBranch = &sManager->branches[i];

        currentBranch->enemyMove = betterBranch->enemyMove;
        currentBranch->enemyScore = betterBranch->enemyScore;
    }
    // 空いた場所に新しいbranch情報を追加
    sManager->branches[idx].enemyMove = pos;
    sManager->branches[idx].enemyScore = score;
}

/**
 * @brief Branchを並び替えつつ挿入
 * 
 * @param sManager 探索マネージャー
 * @param pos 着手位置
 * @param score 着手スコア
 */
void InsertBestBranch(SearchManager *sManager, uint8 pos, score_t score)
{
    BranchProcess *branch;
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        branch = &sManager->branches[i];
        if (branch->enemyScore < score)
        {
            InsertBranch(sManager, pos, score, i);
            break;
        }
    }
}

/**
 * @brief 探索木の情報をすべてのBranchへ共有する
 * 
 * @param sManager 探索マネージャー
 * @param originBranch 情報元のBranch
 */
void BroadcastTreeToAllBranch(SearchManager *sManager, BranchProcess *originBranch)
{
    BranchProcess *branch;
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        branch = &sManager->branches[i];
        if (branch != originBranch)
        {
            TreeClone(originBranch->tree, branch->tree);
        }
    }
}

/**
 * @brief 探索マネージャーを初期化
 * 
 * @param sManager 探索マネージャー
 * @param maxSubProcess 最大プロセス数
 * @param enableAsyncPreSearch 非同期探索を有効にするか
 */
void SearchManagerInit(SearchManager *sManager, int maxSubProcess, bool enableAsyncPreSearch)
{
    DEBUG_PUTS("SearchManager Init\n");
    sManager->numMaxBranches = maxSubProcess;
    sManager->branches = (BranchProcess *)malloc(maxSubProcess * sizeof(BranchProcess));

    sManager->state = SM_WAIT;
    sManager->enableAsyncPreSearching = enableAsyncPreSearch;
    sManager->masterOption = DEFAULT_OPTION;
    sManager->primaryBranch = NULL;

    TreeInit(sManager->shallowTree, true);

    for (int i = 0; i < maxSubProcess; i++)
    {
        BranchInit(&sManager->branches[i], i);
    }
}

/**
 * @brief 探索マネージャーの探索深度設定
 * 
 * @param sManager 探索マネージャー
 * @param mid 中盤探索深度
 * @param end 終盤探索深度
 */
void SearchManagerConfigureDepth(SearchManager *sManager, int mid, int end)
{
    DEBUG_PUTS("SearchManager ConfigureDepth\n");
    DEBUG_PRINTF("\tmid:%d end:%d\n", mid, end);
    sManager->masterOption.midDepth = mid;
    sManager->masterOption.endDepth = end;
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        TreeConfigDepth(sManager->branches[i].tree, mid, end);
    }
}

/**
 * @brief 探索マネージャーの設定
 * 
 * @param sManager 探索マネージャー
 * @param mid 中盤探索深度
 * @param end 終盤探索深度
 * @param oneMoveTime 一手にかける時間
 * @param useIDD 反復深化の利用
 * @param useTimer 時間制限の使用
 * @param useMPC MPCの使用
 */
void SearchManagerConfigure(SearchManager *sManager, int mid, int end, int oneMoveTime, bool useIDD, bool useTimer, bool useMPC)
{
    DEBUG_PUTS("SearchManagerConfigure\n");
    SearchManagerConfigureDepth(sManager, mid, end);

    DEBUG_PRINTF("\ttime:%d useIDD:%d useTimer:%d useMPC:%d\n", oneMoveTime, useIDD, useTimer, useMPC);
    SearchOption *option = &sManager->masterOption;
    option->oneMoveTime = oneMoveTime;
    option->useIDDS = useIDD;
    option->useTimeLimit = useTimer;
    option->useMPC = useMPC;
}

/**
 * @brief 探索マネージャーの解放
 * 
 * @param sManager 探索マネージャー
 */
void SearchManagerDelete(SearchManager *sManager)
{
    DEBUG_PUTS("SearchManagerDelete\n");
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        BranchDelete(&sManager->branches[i]);
    }
    free(sManager->branches);
}

/**
 * @brief 探索マネージャーの探索準備
 * 
 * @param sManager 探索マネージャー
 * @param own 自身の石情報
 * @param opp 相手の石情報
 */
void SearchManagerSetup(SearchManager *sManager, uint64_t own, uint64_t opp)
{
    DEBUG_PUTS("SearchManagerSetup\n");
    SearchManagerKillAll(sManager);
    sManager->stones->own = own;
    sManager->stones->opp = opp;
    sManager->state = SM_WAIT;
}

/**
 * @brief 探索マネージャーのリセット
 * 
 * @param sManager 探索マネージャー
 * @param own 自身の石情報
 * @param opp 相手の石情報
 */
void SearchManagerReset(SearchManager *sManager, uint64_t own, uint64_t opp)
{
    DEBUG_PUTS("SearchManagerReset\n");
    SearchManagerSetup(sManager, own, opp);
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        TreeReset(sManager->branches[i].tree);
    }
}

/**
 * @brief 相手の着手を元に実行中の不要なプロセスを終了させる
 * 
 * @param sManager 探索マネージャー
 * @param enemyPos 相手の着手位置
 * @return BranchProcess* 着手に沿った，生き残りのプロセス
 */
BranchProcess *SearchManagerKillWithoutEnemyPut(SearchManager *sManager, uint8 enemyPos)
{
    DEBUG_PUTS("SearchManagerKillWithoutEnemy\n");
    BranchProcess *branch;
    BranchProcess *primary = NULL;

    // 不要なプロセスの終了フラグを立てる
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        branch = &sManager->branches[i];
        if (branch->state == BRANCH_WAIT)
            continue;
        if (branch->enemyMove == enemyPos)
        {
            DEBUG_PRINTF("\t PrimeSearch Processing @Branch:%d\n", branch->id);
            // メインルートのプロセスは保持
            primary = branch;
            branch->state = BRANCH_PRIME_SEARCH;
            sManager->state = SM_PRIMARY_SEARCH;
        }
        else
        {
            branch->tree->killFlag = true;
        }
    }

    // 終了を待機
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        branch = &sManager->branches[i];
        if (branch->state != BRANCH_WAIT && branch->enemyMove != enemyPos)
        {
            WaitForSingleObject(branch->processHandle, INFINITE);
        }
    }

    // 事前探索プロセス内にメインルートがなかったら待機状態に
    if (primary == NULL)
    {
        sManager->state = SM_WAIT;
    }

    return primary;
}

/**
 * @brief 相手の着手位置決定後のメインルート探索
 * 
 * @param sManager 探索マネージャー
 */
void SearchManagerStartPrimeSearch(SearchManager *sManager)
{
    DEBUG_PUTS("SearchManager StartPrimeSearch\n");
    assert(sManager->state != SM_PRIMARY_SEARCH);

    sManager->state = SM_PRIMARY_SEARCH;
    BranchProcess *branch = sManager->branches;
    sManager->primaryBranch = branch;

    DEBUG_PRINTF("\t PrimeSearch Processing @Branch:%d\n", branch->id);
    DEBUG_PRINTF("\t own:%llu opp:%llu\n", sManager->stones->own, sManager->stones->opp);

    branch->tree->killFlag = false;
    branch->processHandle = NULL;

    branch->state = BRANCH_PRIME_SEARCH;
    {
        TreeConfigClone(branch->tree, sManager->masterOption);
        SearchWithSetup(branch->tree, sManager->stones->own, sManager->stones->opp, false);
    }
    branch->state = BRANCH_WAIT;

    sManager->state = SM_PRIMARY_DONE;
}

/**
 * @brief 並列事前探索の開始
 * 
 * @param sManager 探索マネージャー
 */
void SearchManagerStartPreSearch(SearchManager *sManager)
{
    DEBUG_PUTS("SearchManagerStartPreSearch\n");
    assert(sManager->state == SM_WAIT);
    uint64_t mob = CalcMobility64(sManager->stones->opp, sManager->stones->own);
    int nbMoves = CountBits(mob);

    if (nbMoves == 0)
    {
        return;
    }

    if (nbMoves > 1)
    {
        // 敵盤面で浅い探索をして，スコアマップを作成
        sManager->state = SM_PRE_SORT;
        TreeConfigDepth(sManager->shallowTree, PRE_SEARCH_SHALLO_DEPTH, sManager->masterOption.endDepth);
        SearchWithSetup(sManager->shallowTree, sManager->stones->opp, sManager->stones->own, false);
        // スコアマップの上位いくつかを抽出してBranchに設定
        ResetBranches(sManager);
        for (int pos = 0; pos < 64; pos++)
        {
            InsertBestBranch(sManager, pos, sManager->shallowTree->scoreMap[pos]);
        }
    }
    else
    {
        ResetBranches(sManager);
        InsertBestBranch(sManager, PosIndexFromBit(mob), MAX_VALUE);
    }

    // 各Branchで非同期探索開始
    sManager->state = SM_PRE_SEARCH;
    sManager->primaryBranch = NULL;
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        assert(sManager->branches[i].state == BRANCH_WAIT);
        if (sManager->branches[i].enemyMove != NOMOVE_INDEX)
        {
            sManager->branches[i].state = BRANCH_PRE_SEARCH;
            int depth = sManager->masterOption.useTimeLimit ? PRE_SEARCH_TOO_DEEP_DEPTH : sManager->masterOption.midDepth;
            BranchLaunch(&sManager->branches[i], sManager->stones, depth, sManager->masterOption.endDepth);
        }
    }
}

/**
 * @brief 探索の開始
 * 
 * @param sManager 探索マネージャー
 */
void SearchManagerStartSearch(SearchManager *sManager)
{
    DEBUG_PUTS("SearchManagerStartSearch\n");
    switch (sManager->state)
    {
    case SM_PRE_SEARCH:
        DEBUG_PUTS("\tSM_PRE_SEARCH\n");
        break;

    case SM_PRIMARY_SEARCH:
        DEBUG_PUTS("\tSM_PRIMARY_SEARCH\n");
        // すでに探索実行中
        break;

    case SM_PRIMARY_DONE:
        DEBUG_PUTS("\tSM_PRIMARY_DONE\n");
        // すでに探索済み
        break;

    case SM_WAIT:
        DEBUG_PUTS("\tSM_WAIT\n");
        SearchManagerStartPrimeSearch(sManager);
        break;

    default:
        // Unreachable
        assert(false);

        // fail soft
        SearchManagerKillAll(sManager);
        SearchManagerStartPrimeSearch(sManager);
    }
}

/**
 * @brief 探索情報を戻す
 * 
 * 戻すことは大変なので，すべてのプロセスを終了して石情報を元にリセット
 * 
 * @param sManager 探索マネージャー
 * @param own 自分の石情報
 * @param opp 相手の石情報
 */
void SearchManagerUndo(SearchManager *sManager, uint64_t own, uint64_t opp)
{
    DEBUG_PUTS("SearchManagerUndo\n");
    SearchManagerKillAll(sManager);
    SearchManagerSetup(sManager, own, opp);
}

/**
 * @brief 相手着手時の更新
 * 
 * @param sManager 探索マネージャー
 * @param enemyPos 相手の着手位置
 */
void SearchManagerUpdateOpp(SearchManager *sManager, uint8 enemyPos)
{
    DEBUG_PUTS("SearchManager UpdateOpp\n");
    *sManager->stones = ApplyEnemyPut(sManager->stones, enemyPos);
    if (sManager->enableAsyncPreSearching)
    {
        if (sManager->state == SM_PRE_SEARCH)
        {
            sManager->primaryBranch = SearchManagerKillWithoutEnemyPut(sManager, enemyPos);
        }
        if (sManager->primaryBranch != NULL)
        {
            printf("Found PreSearch!!\n");
        }
    }
}

/**
 * @brief 自身着手時の更新
 * 
 * @param sManager 探索マネージャー
 * @param myPos 自身の着手位置
 */
void SearchManagerUpdateOwn(SearchManager *sManager, uint8 myPos)
{
    DEBUG_PUTS("SearchManager UpdateOwn\n");
    *sManager->stones = ApplyOwnPut(sManager->stones, myPos);
    if (sManager->enableAsyncPreSearching)
    {
        SearchManagerStartPreSearch(sManager);
    }
}

/**
 * @brief 探索結果を取得する
 * 
 * @param sManager 探索マネージャー
 * @param map スコアマップの出力アドレス
 * @return uint8 探索結果-着手位置
 */
uint8 SearchManagerGetMove(SearchManager *sManager, score_t map[64])
{
    DEBUG_PUTS("SearchManager GetMove\n");

    if (sManager->primaryBranch == NULL)
    {
    }

    DEBUG_PRINTF("\t From Branch:%d\n", sManager->primaryBranch->id);
    BranchProcess *primaryBranch = sManager->primaryBranch;

    // 探索中なら待機
    if (sManager->state == SM_PRIMARY_SEARCH && primaryBranch->processHandle)
    {
        if (sManager->masterOption.useTimeLimit)
        {
            DWORD primeThreadState;
            DEBUG_PUTS("\tUse Timer\n");
            GetExitCodeThread(primaryBranch->processHandle, &primeThreadState);
            if (primeThreadState == STILL_ACTIVE)
            {
                DEBUG_PUTS("\tSleep\n");
                Sleep(1000 * sManager->masterOption.oneMoveTime);
            }
        }
        else
        {
            DEBUG_PUTS("\tWaitForSingleObject(primaryBranch->processHandle)\n");
            WaitForSingleObject(primaryBranch->processHandle, INFINITE);
        }
    }

    SearchManagerKillAll(sManager);

    SearchTree *tree = primaryBranch->tree;
    CopyScoreMap(tree->scoreMap, map);
    strcpy(sManager->msg, tree->msg);
    printf("探索深度:%d 思考時間：%.2f[s]  探索ノード数：%.2f[MNode]  探索速度：%.2f[MNode/s]  ",
           tree->completeDepth, tree->usedTime, tree->nodeCount / 1000000.0f, tree->nodeCount / tree->usedTime / 1000000.0f);
    if (tree->isEndSearch)
    {
        printf("推定CPUスコア：%.1f(完)", tree->score);
    }
    else
    {
        printf("推定CPUスコア：%.1f", tree->score / (float)(STONE_VALUE));
    }
    printf("\n");

    sManager->state = SM_WAIT;

    // メイン探索のハッシュ情報など，ツリー情報まるごとクローン
    //BroadcastTreeToAllBranch(sManager, primaryBranch);
    return BestMoveFromMap(map);
}

/**
 * @brief すべてのプロセスを終了する
 * 
 * @param sManager 探索マネージャー
 */
void SearchManagerKillAll(SearchManager *sManager)
{
    DEBUG_PUTS("SearchManager KillAll\n");
    // 終了フラグを立てる
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        BranchProcess *branch = &sManager->branches[i];
        branch->tree->killFlag = true;
    }

    // 終了待ち
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        BranchProcess *branch = &sManager->branches[i];
        if (branch->state != BRANCH_WAIT)
        {
            WaitForSingleObject(branch->processHandle, INFINITE);
            branch->state = BRANCH_WAIT;
            branch->tree->killFlag = false;
        }
    }

    sManager->state = SM_WAIT;
    sManager->primaryBranch = NULL;
}
