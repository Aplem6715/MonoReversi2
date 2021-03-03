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
    uint8 bestMove;
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

void BranchInit(BranchProcess *branch)
{
    TreeInit(branch->tree);
    branch->processHandle = NULL;
    branch->scoreMapMutex = NULL;
    branch->enemyMove = NOMOVE_INDEX;
    branch->state = BRANCH_WAIT;
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
    sManager->numMaxBranches = maxSubProcess;
    sManager->state = SM_WAIT;
    sManager->enableAsyncPreSearching = enableAsyncPreSearch;
    sManager->branches = (BranchProcess *)malloc(maxSubProcess * sizeof(BranchProcess));
    sManager->masterOption = DEFAULT_OPTION;
    sManager->primaryBranch = NULL;

    for (int i = 0; i < maxSubProcess; i++)
    {
        BranchInit(&sManager->branches[i]);
    }
}

void SearchManagerConfigureDepth(SearchManager *sManager, int mid, int end)
{
    sManager->masterOption.midDepth = mid;
    sManager->masterOption.endDepth = end;
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        TreeConfigDepth(sManager->branches[i].tree, mid, end);
    }
}

void SearchManagerConfigure(SearchManager *sManager, int mid, int end, int oneMoveTime, bool useIDD, bool useTimer, bool useMPC)
{
    SearchManagerConfigureDepth(sManager, mid, end);

    SearchOption *option = &sManager->masterOption;
    option->oneMoveTime = oneMoveTime;
    option->useIDDS = useIDD;
    option->useTimeLimit = useTimer;
    option->useMPC = useMPC;
}

void SearchManagerDelete(SearchManager *sManager)
{
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        BranchDelete(&sManager->branches[i]);
    }
    free(sManager->branches);
}

void SearchManagerSetup(SearchManager *sManager, uint64_t own, uint64_t opp)
{
    SearchManagerKillAll(sManager);
    sManager->stones->own = own;
    sManager->stones->opp = opp;
    sManager->state = SM_WAIT;
}

void SearchManagerReset(SearchManager *sManager, uint64_t own, uint64_t opp)
{
    SearchManagerSetup(sManager, own, opp);
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        TreeReset(sManager->branches[i].tree);
    }
}

BranchProcess *SearchManagerKillWithoutEnemyPut(SearchManager *sManager, uint8 enemyPos)
{
    BranchProcess *branch;
    BranchProcess *primary = NULL;
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        branch = &sManager->branches[i];
        if (branch->state == BRANCH_WAIT)
            continue;
        if (branch->enemyMove == enemyPos)
        {
            primary = branch;
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
    assert(sManager->state != SM_PRIMARY_SEARCH);
    sManager->state = SM_PRIMARY_SEARCH;
    BranchProcess *branch = sManager->branches;
    sManager->primaryBranch = branch;

    branch->tree->killFlag = false;
    branch->state = BRANCH_PRIME_SEARCH;
    TreeConfigClone(branch->tree, sManager->masterOption);
    SearchWithSetup(branch->tree, sManager->stones->own, sManager->stones->opp, false);
    branch->state = BRANCH_WAIT;

    CopyScoreMap(branch->tree->scoreMap, sManager->scoreMap);

    sManager->state = SM_WAIT;
}

void SearchManagerStartPreSearch(SearchManager *sManager)
{
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
    switch (sManager->state)
    {
    case SM_PRE_SEARCH:
        break;

    case SM_PRIMARY_SEARCH:
        // すでに探索実行中
        break;

    case SM_WAIT:
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
    SearchManagerKillAll(sManager);
    SearchManagerSetup(sManager, own, opp);
}

void SearchManagerUpdateOpp(SearchManager *sManager, uint8 enemyPos)
{
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
    *sManager->stones = ApplyOwnPut(sManager->stones, myPos);
    if (sManager->enableAsyncPreSearching)
    {
        SearchManagerStartPreSearch(sManager);
    }
}

uint8 SearchManagerGetMove(SearchManager *sManager, score_t map[64])
{
    assert(sManager->primaryBranch != NULL);
    SearchTree *tree = sManager->primaryBranch->tree;
    DWORD primeThreadState;

    if (sManager->masterOption.useTimeLimit)
    {
        GetExitCodeThread(sManager->primaryBranch->processHandle, &primeThreadState);
        if (primeThreadState == STILL_ACTIVE)
        {
            Sleep(1000 * sManager->masterOption.oneMoveTime);
        }
    }
    else
    {
        WaitForSingleObject(sManager->primaryBranch->processHandle, INFINITE);
    }

    SearchManagerKillAll(sManager);
    CopyScoreMap(tree->scoreMap, map);
    printf("探索深度:%d 思考時間：%.2f[s]  探索ノード数：%zu[Node]  探索速度：%.1f[Node/s]  推定CPUスコア：%.1f",
           tree->completeDepth, tree->usedTime, tree->nodeCount, tree->nodeCount / tree->usedTime, tree->score / (float)(STONE_VALUE));
    if (tree->isEndSearch)
    {
        printf("(WLD)");
    }
    printf("\n");
    return BestMoveFromMap(map);
}

void SearchManagerKillAll(SearchManager *sManager)
{
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