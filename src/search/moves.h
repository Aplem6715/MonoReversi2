#ifndef MOVES_DEFINED
#define MOVES_DEFINED

#include "../const.h"

typedef struct Move
{
    uint8 posIdx;
    uint32_t score;
    uint64_t flip;
    struct Move *next;
} Move;

typedef struct MoveList
{
    Move moves[MAX_MOVES + 1];
    uint8 nbMoves;
} MoveList;

struct SearchTree;
struct Stones;
struct HashData;

typedef struct SearchTree SearchTree;
typedef struct Stones Stones;
typedef struct HashData HashData;

void CreateMoveList(MoveList *moveList, Stones *stones);

Move *NextBestMoveWithSwap(Move *prev);

void SortMoveList(MoveList *moveList);

void EvaluateMove(SearchTree *tree, Move *move, Stones *stones, score_t alpha, uint8 shallowDepth, const HashData *hashData);

void EvaluateMoveList(SearchTree *tree, MoveList *movelist, Stones *stones, score_t alpha, uint8 depth, const HashData *hashData);

#endif