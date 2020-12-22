#ifndef GAME_DEFINED
#define GAME_DEFINED

#include "board.h"
#include "search/search.h"

enum PlayerEnum
{
    HUMAN,
    AI,
};

class Game
{
private:
    PlayerEnum player[2];
    SearchTree tree[2];
    Board board;

public:
    Game(PlayerEnum white, PlayerEnum black);
    ~Game();
    uint64 Game::WaitPosAI(uint8 color);
    uint64 Game::WaitPos(uint8 color);

    void Start();
};

#endif