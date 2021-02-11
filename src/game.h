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

class Game
{
private:
    PlayerEnum player[2];
    SearchTree tree[2];
    Board board[1];
    uint8 turn;
    std::vector<uint8> moves;

public:
    Game(PlayerEnum black, PlayerEnum white, int mid, int end);
    ~Game();
    uint8 Game::WaitPosAI(uint8 color);
    uint8 Game::WaitPos(uint8 color);

    void Reset();
    void Start();
    SearchTree *GetTree(uint8 color);
};

void LoadGameRecords(const char *file, std::vector<std::vector<uint8>> &moves);

#endif