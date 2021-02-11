#include <time.h>
#include "search/search.h"
#include "board.h"
#include "bit_operation.h"

#ifdef _WINDLL

#ifndef DLLAPI
#define DLLAPI __declspec(dllexport)
#endif

extern "C"
{
    enum GUI_TextColor
    {
        GUI_BLACK,
        GUI_WHITE,
        GUI_LIME,
        GUI_ORANGE,
        GUI_RED
    };

    typedef const void(__stdcall *GUI_Log)(int knd, char *str);

    GUI_Log GUI_Print;
    SearchTree dllTree[1];
    Board dllBoard[1];

    DLLAPI void DllInit();
    DLLAPI void DllConfigureSearch(unsigned char midDepth, unsigned char endDepth);
    DLLAPI int DllSearch(double *value);

    DLLAPI void DllBoardReset();
    DLLAPI int DllPut(int pos);
    DLLAPI int DllStoneCount(int color);
    DLLAPI int DllUndo();
    DLLAPI int DllIsGameEnd();
    DLLAPI int DllIsLegal(int pos);
    DLLAPI uint64_t DllGetMobility();
    DLLAPI uint64_t DllGetMobilityC(int color);
    DLLAPI uint64_t DllGetStones(int color);

    DLLAPI void SetCallBack(GUI_Log printCallback);
    DLLAPI void DllShowMsg();

    void DllInit()
    {
        srand((unsigned int)time(NULL));
        HashInit();
        InitTree(dllTree, 8, 20, 4, 8, 1, 1, 1);
        BoardReset(dllBoard);
    }

    void DllConfigureSearch(unsigned char midDepth, unsigned char endDepth)
    {
        ConfigTree(dllTree, midDepth, endDepth);
    }

    int DllSearch(double *value)
    {
        return Search(dllTree, BoardGetOwn(dllBoard), BoardGetOpp(dllBoard), 0);
    }

    void DllBoardReset()
    {
        ResetTree(dllTree);
        BoardReset(dllBoard);
    }

    int DllPut(int pos)
    {
        if (BoardIsLegalTT(dllBoard, pos))
        {
            BoardPutTT(dllBoard, pos);
            if (BoardGetMobility(dllBoard) == 0)
            {
                BoardSkip(dllBoard);
            }
            return 1;
        }
        return 0;
    }

    int DllStoneCount(int color)
    {
        return BoardGetStoneCount(dllBoard, color);
    }

    int DllUndo()
    {
        if (BoardUndo(dllBoard))
        {
            return 1;
        }
        return 0;
    }

    int DllIsGameEnd()
    {
        return BoardIsFinished(dllBoard);
    }

    int DllIsLegal(int pos)
    {
        return BoardIsLegalTT(dllBoard, pos);
    }

    uint64_t DllGetMobility()
    {
        return BoardGetMobility(dllBoard);
    }

    uint64_t DllGetMobilityC(int color)
    {
        return BoardGetMobility(dllBoard, color);
    }

    uint64_t DllGetStones(int color)
    {
        if (color == BLACK)
        {
            return BoardGetBlack(dllBoard);
        }
        else
        {
            return BoardGetWhite(dllBoard);
        }
    }

    void SetCallBack(GUI_Log printCallback)
    {
        GUI_Print = printCallback;
        GUI_Print(GUI_ORANGE, "System: Callback Initialized");
    }

    void DllShowMsg()
    {
        GUI_Print(GUI_LIME, dllTree->msg);
    }
}
#endif