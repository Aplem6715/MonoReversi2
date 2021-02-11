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

typedef struct Game
{
    PlayerEnum player[2];
    SearchTree tree[2];
    Board board[1];
    uint8 turn;
    std::vector<uint8> moves;
} Game;

void LoadGameRecords(const char *file, std::vector<std::vector<uint8>> &moves);

void GameInit(Game *game, PlayerEnum black, PlayerEnum white, int mid, int end);
void GameFree(Game *game);
uint8 GameWaitPosAI(Game *game, uint8 color);
uint8 GameWaitPos(Game *game, uint8 color);

void GameReset(Game *game);
void GameStart(Game *game);
SearchTree *GameGetTree(Game *game, uint8 color);

#endif