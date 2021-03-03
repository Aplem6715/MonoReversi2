
#ifndef GAME_DEFINED
#define GAME_DEFINED

#include "board.h"
#include "search/search_manager.h"

typedef enum GameMode
{
    GM_CPU_BLACK,
    GM_CPU_WHITE,
    GM_PVP
} GameMode;

typedef struct Game
{
    GameMode mode;
    //SearchTree tree[2];
    SearchManager sManager[1];
    Board board[1];
    uint8 turn;
    uint8 moves[60];
} Game;

void GameInit(Game *game, GameMode mode, int mid, int end);
void GameFree(Game *game);
uint8 GameWaitPosAI(Game *game, uint8 color);
uint8 GameWaitPos(Game *game, uint8 color);

void GameReset(Game *game);
void GameStart(Game *game);
//SearchTree *GameGetTree(Game *game, uint8 color);

#endif