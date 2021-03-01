#if !defined(_SEARCH_MANAGER_H_)
#define _SEARCH_MANAGER_H_

#include "../const.h"
#include "../board.h"
#include "search.h"

#include <windows.h>
#include <process.h>

typedef enum SearchMangerState
{
    SM_PRE_SORT,
    SM_PRE_SEARCH,
    SM_PRIMARY_SEARCH,
    SM_WAIT,
} SearchMangerState;

typedef struct BranchProcess
{
    SearchTree *tree;
    HANDLE processHandle;
    HANDLE scoreMapMutex;

    uint8 enemyMove;
} BranchProcess;

typedef struct SearchManager
{
    Board board[1];
    BranchProcess *branches;
    SearchMangerState state;

    int numMaxBranches;
    int numBranches;

    bool enableAsyncPreSearching;
} SearchManager;

void SearchManagerSetup(uint64_t own, uint64_t opp);
void SearchManagerStartSearch();
void SearchManagerKillWithEnemyPut(uint8 pos);
int SearchManagerGetScoreMap(score_t map[64]);
void SearchManagerKillAll();

#endif // _SEARCH_MANAGER_H_
