#ifndef MOVES_DEFINED
#define MOVES_DEFINED

#include "hash.h"
#include "../const.h"

typedef struct Move
{
    uint64 flip;
    uint8 posIdx;
    uint32 score;
    Move *next;
} Move;

typedef struct MoveList
{
    Move moves[MAX_MOVES + 1];
    uint8 nbMoves;
} MoveList;

struct SearchTree;

void CreateMoveList(MoveList *moveList, uint64 own, uint64 opp);
void EvaluateMove(SearchTree *tree, Move *move, uint64 own, uint64 opp, float alpha, const HashData *hashData);
void EvaluateMoveList(SearchTree *tree, MoveList *movelist, uint64 own, uint64 opp, float alpha, const HashData *hashData);
Move *NextBestMoveWithSwap(Move *prev);
void SortMoveList(MoveList *moveList);

#endif