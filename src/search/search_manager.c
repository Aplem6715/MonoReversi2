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
#include <assert.h>
#include <stdio.h>

#include "search_manager.h"
#include "../bit_operation.h"
#include "../debug_util.h"

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

void CopyScoreMap(score_t src[64], score_t dst[64])
{
    for (int i = 0; i < 64; i++)
    {
        dst[i] = src[i];
    }
}

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

void BranchInit(BranchProcess *branch, int id)
{
    TreeInit(branch->tree);
    branch->processHandle = NULL;
    branch->scoreMapMutex = NULL;
    branch->enemyMove = NOMOVE_INDEX;
    branch->state = BRANCH_WAIT;
    branch->id = id;
}

void BranchDelete(BranchProcess *branch)
{
    TreeDelete(branch->tree);
}

void StartSearchAsync(void *tree)
{
    SearchWithoutSetup((SearchTree *)tree);
    _endthread();
}

void BranchLaunch(BranchProcess *branch, Stones *beforeEnemyStones, int depth)
{
    Stones stones = ApplyEnemyPut(beforeEnemyStones, branch->enemyMove);
    branch->tree->killFlag = false;

    // 事前探索をするときはタイマーOFF
    TreeConfig(branch->tree, depth, 20, 10000, true, false, false);
    SearchSetup(branch->tree, stones.own, stones.opp);

    branch->processHandle = (HANDLE)_beginthread(StartSearchAsync, 0, branch->tree);
}

void ResetBranches(SearchManager *sManager)
{
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        sManager->branches[i].enemyMove = NOMOVE_INDEX;
        sManager->branches[i].enemyScore = MIN_VALUE;
    }
}

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

void SearchManagerInit(SearchManager *sManager, int maxSubProcess, bool enableAsyncPreSearch)
{
    DEBUG_PUTS("SearchManager Init\n");
    sManager->numMaxBranches = maxSubProcess;
    sManager->state = SM_WAIT;
    sManager->enableAsyncPreSearching = enableAsyncPreSearch;
    sManager->branches = (BranchProcess *)malloc(maxSubProcess * sizeof(BranchProcess));
    sManager->masterOption = DEFAULT_OPTION;
    sManager->primaryBranch = NULL;

    for (int i = 0; i < maxSubProcess; i++)
    {
        BranchInit(&sManager->branches[i], i);
    }
}

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

void SearchManagerDelete(SearchManager *sManager)
{
    DEBUG_PUTS("SearchManagerDelete\n");
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        BranchDelete(&sManager->branches[i]);
    }
    free(sManager->branches);
}

void SearchManagerSetup(SearchManager *sManager, uint64_t own, uint64_t opp)
{
    DEBUG_PUTS("SearchManagerSetup\n");
    SearchManagerKillAll(sManager);
    sManager->stones->own = own;
    sManager->stones->opp = opp;
    sManager->state = SM_WAIT;
}

void SearchManagerReset(SearchManager *sManager, uint64_t own, uint64_t opp)
{
    DEBUG_PUTS("SearchManagerReset\n");
    SearchManagerSetup(sManager, own, opp);
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        TreeReset(sManager->branches[i].tree);
    }
}

BranchProcess *SearchManagerKillWithoutEnemyPut(SearchManager *sManager, uint8 enemyPos)
{
    DEBUG_PUTS("SearchManagerKillWithoutEnemy\n");
    BranchProcess *branch;
    BranchProcess *primary = NULL;
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        branch = &sManager->branches[i];
        if (branch->state == BRANCH_WAIT)
            continue;
        if (branch->enemyMove == enemyPos)
        {
            DEBUG_PRINTF("\t PrimeSearch Processing @Branch:%d\n", branch->id);
            primary = branch;
            branch->state = BRANCH_PRIME_SEARCH;
            sManager->state = SM_PRIMARY_SEARCH;
        }
        else
        {
            branch->tree->killFlag = true;
        }
    }

    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        branch = &sManager->branches[i];
        if (branch->state != BRANCH_WAIT && branch->enemyMove != enemyPos)
        {
            WaitForSingleObject(branch->processHandle, INFINITE);
        }
    }
    return primary;
}

void SearchManagerStartPrimeSearch(SearchManager *sManager)
{
    DEBUG_PUTS("SearchManager StartPrimeSearch\n");
    assert(sManager->state != SM_PRIMARY_SEARCH);

    sManager->state = SM_PRIMARY_SEARCH;
    BranchProcess *branch = sManager->branches;

    DEBUG_PRINTF("\t PrimeSearch Processing @Branch:%d\n", branch->id);
    DEBUG_PRINTF("\t own:%llu opp:%llu\n", sManager->stones->own, sManager->stones->opp);

    sManager->primaryBranch = branch;

    branch->tree->killFlag = false;
    branch->processHandle = NULL;

    branch->state = BRANCH_PRIME_SEARCH;
    TreeConfigClone(branch->tree, sManager->masterOption);
    SearchWithSetup(branch->tree, sManager->stones->own, sManager->stones->opp, false);
    branch->state = BRANCH_WAIT;

    //CopyScoreMap(branch->tree->scoreMap, sManager->scoreMap);

    sManager->state = SM_PRIMARY_DONE;
}

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
        BranchProcess *branch = sManager->branches;
        // 敵盤面で浅い探索をして，スコアマップを作成
        sManager->state = SM_PRE_SORT;
        branch->tree->killFlag = false;
        TreeConfigDepth(branch->tree, 6, 10);
        SearchWithSetup(branch->tree, sManager->stones->opp, sManager->stones->own, false);
        // スコアマップの上位いくつかを抽出してBranchに設定
        ResetBranches(sManager);
        for (int pos = 0; pos < 64; pos++)
        {
            InsertBestBranch(sManager, pos, branch->tree->scoreMap[pos]);
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
            int depth = sManager->masterOption.useTimeLimit ? 20 : sManager->masterOption.midDepth;
            sManager->branches[i].state = BRANCH_PRE_SEARCH;
            BranchLaunch(&sManager->branches[i], sManager->stones, depth);
        }
    }
}

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

void SearchManagerUndo(SearchManager *sManager, uint64_t own, uint64_t opp)
{
    DEBUG_PUTS("SearchManagerUndo\n");
    SearchManagerKillAll(sManager);
    SearchManagerSetup(sManager, own, opp);
}

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
        if (sManager->primaryBranch == NULL)
        {
            SearchManagerStartPrimeSearch(sManager);
        }
        else
        {
            printf("Found PreSearch!!\n");
        }
    }
}

void SearchManagerUpdateOwn(SearchManager *sManager, uint8 myPos)
{
    DEBUG_PUTS("SearchManager UpdateOwn\n");
    *sManager->stones = ApplyOwnPut(sManager->stones, myPos);
    if (sManager->enableAsyncPreSearching)
    {
        SearchManagerStartPreSearch(sManager);
    }
}

uint8 SearchManagerGetMove(SearchManager *sManager, score_t map[64])
{
    DEBUG_PUTS("SearchManager GetMove\n");
    assert(sManager->primaryBranch != NULL);
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
    printf("探索深度:%d 思考時間：%.2f[s]  探索ノード数：%zu[Node]  探索速度：%.1f[Node/s]  推定CPUスコア：%.1f",
           tree->completeDepth, tree->usedTime, tree->nodeCount, tree->nodeCount / tree->usedTime, tree->score / (float)(STONE_VALUE));
    if (tree->isEndSearch)
    {
        printf("(WLD)");
    }
    printf("\n");

    sManager->state = SM_WAIT;
    return BestMoveFromMap(map);
}

void SearchManagerKillAll(SearchManager *sManager)
{
    DEBUG_PUTS("SearchManager KillAll\n");
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        BranchProcess *branch = &sManager->branches[i];
        branch->tree->killFlag = true;
    }

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