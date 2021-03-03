#if !defined(_SEARCH_MANAGER_H_)
#define _SEARCH_MANAGER_H_

#undef D8
#include <process.h>
#include <windows.h>

#include "../const.h"
#include "search.h"

#define DEFAULT_PROCESS_NUM 4

typedef enum BranchState
{
    BRANCH_WAIT,
    BRANCH_PRE_SEARCH,
    BRANCH_PRIME_SEARCH,
} BranchState;

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
    BranchState state;

    uint8 enemyMove;
    score_t enemyScore;
} BranchProcess;

typedef struct SearchManager
{
    Stones stones[1];
    BranchProcess *branches;
    BranchProcess *primaryBranch;
    SearchMangerState state;
    SearchOption masterOption;

    score_t scoreMap[64];

    int numMaxBranches;
    int numBranches;

    bool enableAsyncPreSearching;
} SearchManager;

void SearchManagerInit(SearchManager *sManager, int maxSubProcess, bool enableAsyncPreSearch);
void SearchManagerConfigure(SearchManager *sManager, int mid, int end);
void SearchManagerDelete(SearchManager *sManager);
void SearchManagerSetup(SearchManager *sManager, uint64_t own, uint64_t opp);
BranchProcess *SearchManagerKillWithoutEnemyPut(SearchManager *sManager, uint8 enemyPos);
void SearchManagerStartPrimeSearch(SearchManager *sManager);
void SearchManagerStartPreSearch(SearchManager *sManager);
void SearchManagerStartSearch(SearchManager *sManager);
void SearchManagerUndo(SearchManager *sManager, uint64_t own, uint64_t opp);
void SearchManagerUpdateOpp(SearchManager *sManager, uint8 enemyPos);
void SearchManagerUpdateOwn(SearchManager *sManager, uint8 myPos);
uint8 SearchManagerGetMove(SearchManager *sManager, score_t map[64]);
void SearchManagerKillAll(SearchManager *sManager);

#endif // _SEARCH_MANAGER_H_
