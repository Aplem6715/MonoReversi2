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
    Board();
    ~Board();

    static void Draw(uint64 black, uint64 white, uint64 mobility);

    uint64 GetBlack();
    uint64 GetWhite();
    uint64 GetOwn();
    uint64 GetOpp();
    uint64 GetMobility();
    uint8 GetTurnColor();
    void Reset();
    uint64 PutTT(uint8 pos);
    uint8 GetRandomPosMoveable();
    void Undo();
    void UndoUntilColorChange();
    void Skip();
    void Draw();
    int GetStoneCount(uint8 color);
    bool IsLegalTT(uint8 pos);
    bool IsFinished();
};

#endif