#include "search_manager.h"

void BranchInit(BranchProcess *branch)
{
    TreeInit(branch->tree);
}

void SearchManagerInit(SearchManager *sManager, int maxSubProcess)
{
    sManager->numMaxBranches = maxSubProcess;
}