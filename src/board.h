#ifndef BOARD_DEFINED
#define BOARD_DEFINED

#include "const.h"

const int HIST_LENGTH = 60;

typedef struct History
{
    uint64_t flip;
    uint64_t pos;
    uint8 color;
} History;

class Board
{
private:
    uint64_t white;
    uint64_t black;
    uint8 turn;
    History history[HIST_LENGTH];
    int nbPlayed;

public:
    Board();
    ~Board();

    static void Draw(uint64_t black, uint64_t white, uint64_t mobility);

    uint64_t GetBlack();
    uint64_t GetWhite();
    uint64_t GetOwn();
    uint64_t GetOpp();
    uint64_t GetMobility();
    uint64_t GetMobility(uint8 color);
    uint8 GetTurnColor();
    void Reset();
    void SetStones(uint64_t black, uint64_t white, uint8 turn);
    uint64_t PutTT(uint8 pos);
    uint8 GetRandomPosMoveable();
    int Undo();
    void UndoUntilColorChange();
    void Skip();
    void Draw();
    int GetStoneCount(uint8 color);
    bool IsLegalTT(uint8 pos);
    bool IsFinished();
};

#endif