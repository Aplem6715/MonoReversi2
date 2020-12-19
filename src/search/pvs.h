#ifndef PVS_DEFINED
#define PVS_DEFINED

#include "search/ab_node.h"

void pvs_root(uint64 own, uint64 opp, unsigned char depth);

void pvs(AbNode* node);

#endif