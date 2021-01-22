#ifndef MOVES_DEFINED
#define MOVES_DEFINED

#include "../stones.h"
#include "hash.h"
#include "../const.h"

typedef struct Move
{
    uint64_t flip;
    uint8 posIdx;
    uint32_t score;
    Move *next;
} Move;

typedef struct MoveList
{
    Move moves[MAX_MOVES + 1];
    uint8 nbMoves;
} MoveList;

struct SearchTree;

void CreateMoveList(MoveList *moveList, Stones *stones);
void EvaluateMove(SearchTree *tree, Move *move, Stones *stones, const HashData *hashData);
void EvaluateMoveList(SearchTree *tree, MoveList *movelist, Stones *stones, const HashData *hashData);
Move *NextBestMoveWithSwap(Move *prev);
void SortMoveList(MoveList *moveList);

#endif