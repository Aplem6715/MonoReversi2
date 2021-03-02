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

void ApplyEnemyPut(Stones *stones, uint8 enemyPos)
{
    uint64_t flip = CalcFlip64(stones->opp, stones->own, enemyPos);
    uint64_t posBit = CalcPosBit(enemyPos);
    stones->opp ^= flip | posBit;
    stones->own ^= flip;
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
}

void BranchDelete(BranchProcess *branch)
{
    TreeDelete(branch->tree);
}

void BranchLaunchAsync(BranchProcess *branch)
{
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
        if (branch->enemyMove != enemyPos)
        {
            branch->tree->killFlag = true;
        }
    }
}

void SearchManagerStartPrimeSearch(SearchManager *sManager)
{
    assert(sManager->state == SM_WAIT);
    BranchProcess *branch = sManager->branches;

    Search(branch->tree, sManager->stones->own, sManager->stones->opp, false);
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
    Search(branch->tree, sManager->stones->opp, sManager->stones->own, false);

    // スコアマップの上位いくつかを抽出してBranchに設定
    ResetBranches(sManager);
    for (int pos = 0; pos < 64; pos++)
    {
        InsertBestBranch(sManager, pos, branch->tree->scoreMap[pos]);
    }

    // 各Branchで非同期探索開始
    sManager->state = SM_PRE_SEARCH;
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        BranchLaunchAsync(&sManager->branches[i]);
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
        ApplyEnemyPut(sManager->stones, enemyPos);
        SearchManagerStartPrimeSearch(sManager);
        break;

    default:
        // Unreachable
        assert(true);

        // fail soft
        ApplyEnemyPut(sManager->stones, enemyPos);
        SearchManagerKillAll(sManager);
        SearchManagerStartPrimeSearch(sManager);
    }
}

int SearchManagerGetScoreMap(SearchManager *sManager, score_t map[64]);

void SearchManagerKillAll(SearchManager *sManager)
{
    for (int i = 0; i < sManager->numMaxBranches; i++)
    {
        BranchProcess *branch = &sManager->branches[i];
        branch->tree->killFlag = true;
    }
    sManager->state = SM_WAIT;
}