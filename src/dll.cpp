#include "search/search.h"
#include "board.h"
#include "bit_operation.h"

#ifdef _WINDLL

#ifndef DLLAPI
#define DLLAPI __declspec(dllexport)
#endif

SearchTree dllTree[1];
Board dllBoard[1];

DLLAPI void DllInit();
DLLAPI void DllConfigureSearch(unsigned char midDepth, unsigned char endDepth);
DLLAPI uint8 DllSearch(double *value);

DLLAPI void DllReset();
DLLAPI void DllPut(uint8 pos);
DLLAPI uint8 DllStoneCount(uint8 color);
DLLAPI void DllUndo();
DLLAPI uint8 DllIsGameEnd();
DLLAPI uint64_t DllGetMobility();
DLLAPI uint64_t DllGetStones(uint8 color);

DLLAPI void DllInit()
{
    InitTree(dllTree, 8, 20, 4, 8, 1);
    dllBoard->Reset();
}

DLLAPI void DllConfigureSearch(unsigned char midDepth, unsigned char endDepth)
{
    ConfigTree(dllTree, midDepth, endDepth);
}

DLLAPI uint8 DllSearch(double *value)
{
    return Search(dllTree, dllBoard->GetOwn(), dllBoard->GetOpp(), 0);
}

DLLAPI void DllReset()
{
    ResetTree(dllTree);
    dllBoard->Reset();
}

DLLAPI void DllPut(uint8 pos)
{
    dllBoard->PutTT(pos);
}

DLLAPI uint8 DllStoneCount(uint8 color)
{
    return dllBoard->GetStoneCount(color);
}

DLLAPI void DllUndo()
{
    dllBoard->Undo();
}

DLLAPI uint8 DllIsGameEnd()
{
    return dllBoard->IsFinished();
}

DLLAPI uint64_t DllGetMobility()
{
    return dllBoard->GetMobility();
}

DLLAPI uint64_t DllGetStones(uint8 color)
{
    if (color == Const::BLACK)
    {
        return dllBoard->GetBlack();
    }
    else
    {
        return dllBoard->GetWhite();
    }
}

#endif