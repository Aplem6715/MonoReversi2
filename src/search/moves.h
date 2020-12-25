#ifndef MOVES_DEFINED
#define MOVES_DEFINED

#include "s_const.h"
#include "hash.h"

typedef struct Move
{
    uint64 flip;
    uint8 posIdx;
    int score;
    Move *next;
} Move;

typedef struct MoveList
{
    Move moves[MAX_MOVES + 1];
    uint8 nbMoves;
} MoveList;

void CreateMoveList(MoveList *moveList, uint64 own, uint64 opp);
void EvaluateMove(Move *move, uint64 own, uint64 opp, const HashData *hashData);
void EvaluateMoveList(MoveList *movelist, uint64 own, uint64 opp, const HashData *hashData);
Move *NextBestMoveWithSwap(Move *prev);
void SortMoveList(MoveList *moveList);

#endif