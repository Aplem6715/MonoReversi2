
#ifndef GAME_DEFINED
#define GAME_DEFINED

#include "board.h"
#include "search/search.h"

typedef enum PlayerEnum
{
    HUMAN,
    AI,
} PlayerEnum;

typedef struct Game
{
    PlayerEnum player[2];
    SearchTree tree[2];
    Board board[1];
    uint8 turn;
    uint8 moves[60];
} Game;

void GameInit(Game *game, PlayerEnum black, PlayerEnum white, int mid, int end);
void GameFree(Game *game);
uint8 GameWaitPosAI(Game *game, uint8 color);
uint8 GameWaitPos(Game *game, uint8 color);

void GameReset(Game *game);
void GameStart(Game *game);
SearchTree *GameGetTree(Game *game, uint8 color);

#endif