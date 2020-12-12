#ifndef GAME_DEFINED
#define GAME_DEFINED

#include "board.h"

enum GameMode
{
    HUMAN_VS_HUMAN,
    HUMAN_VS_CPU,
    CPU_VS_CPU,
};

class Game
{
private:
    GameMode mode;
    Board board;

public:
    Game(GameMode mode);
    ~Game();

    void Start();
};

#endif