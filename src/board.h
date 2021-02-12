#ifndef BOARD_DEFINED
#define BOARD_DEFINED

#include "const.h"

#define HIST_LENGTH 60

typedef struct History
{
    uint64_t flip;
    uint64_t pos;
    uint8 color;
} History;

typedef struct Board
{
    uint64_t white;
    uint64_t black;
    uint8 turn;
    History history[HIST_LENGTH];
    int nbPlayed;
} Board;

extern void Draw(uint64_t black, uint64_t white, uint64_t mobility);

uint64_t BoardGetBlack(Board *board);
uint64_t BoardGetWhite(Board *board);
uint64_t BoardGetOwn(Board *board);
uint64_t BoardGetOpp(Board *board);
uint64_t BoardGetMobility(Board *board);
uint64_t BoardGetColorsMobility(Board *board, uint8 color);
uint8 BoardGetTurnColor(Board *board);
void BoardReset(Board *board);
void BoardSetStones(Board *board, uint64_t black, uint64_t white, uint8 turn);
uint64_t BoardPutTT(Board *board, uint8 pos);
uint8 BoardGetRandomPosMoveable(Board *board);
int BoardUndo(Board *board);
void BoardUndoUntilColorChange(Board *board);
void BoardSkip(Board *board);
void BoardDraw(Board *board);
int BoardGetStoneCount(Board *board, uint8 color);
bool BoardIsLegalTT(Board *board, uint8 pos);
bool BoardIsFinished(Board *board);

#endif