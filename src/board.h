#ifndef BOARD_DEFINED
#define BOARD_DEFINED

#include "const.h"

const int HIST_LENGTH = 60;

typedef struct History
{
    uint64 flip;
    uint64 pos;
    uint8 color;
} History;

class Board
{
private:
    uint64 white;
    uint64 black;
    uint8 turn;
    History history[HIST_LENGTH];
    int nbPlayed;

public:
    Board(/* args */);
    ~Board();

    uint64 GetBlack();
    uint64 GetWhite();
    uint64 GetOwn();
    uint64 GetOpp();
    uint64 GetMobility();
    uint8 GetTurnColor();
    void Reset();
    uint64 Put(uint64 pos);
    uint64 GetRandomPosMoveable();
    void Undo();
    void UndoUntilColorChange();
    void Skip();
    void Draw();
    int GetStoneCount(uint8 color);
    bool IsLegal(uint64 pos);
    bool IsFinished();
};

#endif