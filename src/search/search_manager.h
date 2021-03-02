#if !defined(_SEARCH_MANAGER_H_)
#define _SEARCH_MANAGER_H_

#include "../const.h"
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
    SearchTree tree[1];
    HANDLE processHandle;
    HANDLE scoreMapMutex;

    uint8 enemyMove;
} BranchProcess;

typedef struct SearchManager
{
    Stones stones[1];
    BranchProcess *branches;
    SearchMangerState state;

    int numMaxBranches;
    int numBranches;

    bool enableAsyncPreSearching;
} SearchManager;

void SearchManagerInit(SearchManager *sManager, int maxSubProcess);
void SearchManagerDelete(SearchManager *sManager);
void SearchManagerSetup(SearchManager *sManager, uint64_t own, uint64_t opp);
void SearchManagerKillWithoutEnemyPut(SearchManager *sManager, uint8 enemyPos);
void SearchManagerStartPrimeSearch(SearchManager *sManager);
void SearchManagerStartPreSearch(SearchManager *sManager);
void SearchManagerStartSearch(SearchManager *sManager, uint8 enemyPos);
int SearchManagerGetScoreMap(SearchManager *sManager, score_t map[64]);
void SearchManagerKillAll(SearchManager *sManager);

#endif // _SEARCH_MANAGER_H_
