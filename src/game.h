#ifndef GAME_DEFINED
#define GAME_DEFINED

#include <vector>
#include "board.h"
#include "search/search.h"

enum PlayerEnum
{
    HUMAN,
    AI,
};

void LoadGameRecords(char *file, std::vector<std::vector<uint8>> &moves);

class Game
{
private:
    PlayerEnum player[2];
    SearchTree tree[2];
    Board board;
    uint8 turn;
    std::vector<uint8> moves;

public:
    Game(PlayerEnum white, PlayerEnum black);
    ~Game();
    uint64 Game::WaitPosAI(uint8 color);
    uint64 Game::WaitPos(uint8 color);

    void Reset();
    void Start();
};

#endif