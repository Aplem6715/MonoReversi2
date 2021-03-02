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

Stones ApplyEnemyPut(Stones *stones, uint8 enemyPos)
{
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

void BranchLaunch(BranchProcess *branch, Stones *beforeEnemyStones)
{
    Stones stones = ApplyEnemyPut(beforeEnemyStones, branch->enemyMove);
    SearchSetup(branch->tree, stones.own, stones.opp);

    branch->processHandle = (HANDLE)_beginthread(StartSearchAsync, 0, branch->tree);
}

void SearchManagerInit(SearchManager *sManager, int maxSubProcess, bool enableAsyncPreSearch)
{
    sManager->numMaxBranches = maxSubProcess;
    sManager->state = SM_WAIT;
    sManager->enableAsyncPreSearching = enableAsyncPreSearch;
    sManager->branches = (BranchProcess *)malloc(maxSubProcess * sizeof(BranchProcess));
    for (int i = 0; i < maxSubProcess; i++)
    {
        BranchInit(&sManager->branches[i]);
    }
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

void SearchManagerKillWithoutEnemyPut(SearchManager *sManager, uint8 enemyPos)
{
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        BranchProcess *branch = &sManager->branches[i];
        if (branch->enemyMove == enemyPos)
        {
            sManager->primaryBranch = branch;
        }
        else
        {
            branch->tree->killFlag = true;
        }
    }
    sManager->state = SM_PRIMARY_SEARCH;
}

void SearchManagerStartPrimeSearch(SearchManager *sManager)
{
    assert(sManager->state == SM_WAIT);
    sManager->state = SM_PRIMARY_SEARCH;
    BranchProcess *branch = sManager->branches;
    sManager->primaryBranch = branch;

    branch->state = BRANCH_PRIME_SEARCH;
    SearchWithSetup(branch->tree, sManager->stones->own, sManager->stones->opp, false);
    branch->state = BRANCH_WAIT;

    CopyScoreMap(branch->tree->scoreMap, sManager->scoreMap);

    sManager->state = SM_WAIT;
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
    // 終端からidxまでをシフトして，idx位置のbranchを開ける
    for (int i = sManager->numMaxBranches - 1; i > idx; i--)
    {
        BranchProcess *betterBranch = &sManager->branches[i - 1];
        BranchProcess *currentBranch = &sManager->branches[i];

        currentBranch->enemyMove = betterBranch->enemyMove;
        currentBranch->enemyScore = betterBranch->enemyScore;
    }
    // 空いた場所に新しいbranch情報を追加
    sManager->branches[idx].enemyMove = pos;
    sManager->branches[idx].enemyScore = score;
}

void InsertBestBranch(SearchManager *sManager, uint8 pos, score_t score)
{
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        BranchProcess *branch = &sManager->branches[i];
        if (branch->enemyScore < score)
        {
            InsertBranch(sManager, pos, score, i);
            break;
        }
    }
}

void SearchManagerStartPreSearch(SearchManager *sManager)
{
    assert(sManager->state == SM_WAIT);
    BranchProcess *branch = sManager->branches;

    // 敵盤面で浅い探索をして，スコアマップを作成
    sManager->state = SM_PRE_SORT;
    SearchWithSetup(branch->tree, sManager->stones->opp, sManager->stones->own, false);

    // スコアマップの上位いくつかを抽出してBranchに設定
    ResetBranches(sManager);
    for (int pos = 0; pos < 64; pos++)
    {
        InsertBestBranch(sManager, pos, branch->tree->scoreMap[pos]);
    }

    // 各Branchで非同期探索開始
    sManager->state = SM_PRE_SEARCH;
    sManager->primaryBranch = NULL;
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        sManager->branches[i].state = BRANCH_PRE_SEARCH;
        BranchLaunch(&sManager->branches[i], sManager->stones);
    }
}

void SearchManagerStartSearch(SearchManager *sManager, uint8 enemyPos)
{
    switch (sManager->state)
    {
    case SM_PRE_SEARCH:
        SearchManagerKillWithoutEnemyPut(sManager, enemyPos);
        break;

    case SM_WAIT:
        *sManager->stones = ApplyEnemyPut(sManager->stones, enemyPos);
        SearchManagerStartPrimeSearch(sManager);
        break;

    default:
        // Unreachable
        assert(true);

        // fail soft
        *sManager->stones = ApplyEnemyPut(sManager->stones, enemyPos);
        SearchManagerKillAll(sManager);
        SearchManagerStartPrimeSearch(sManager);
    }
}

int SearchManagerGetScoreMap(SearchManager *sManager, score_t map[64])
{
    SearchManagerKillAll(sManager);
    CopyScoreMap(sManager->primaryBranch->tree->scoreMap, map);
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
        WaitForSingleObject(sManager->branches[i].processHandle, INFINITE);
        sManager->branches[i].state = BRANCH_WAIT;
    }
    sManager->state = SM_WAIT;
}