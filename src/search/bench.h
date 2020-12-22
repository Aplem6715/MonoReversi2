#ifndef BENCH_DEFINED
#define BENCH_DEFINED

#include "../const.h"

#define SEARCH_BENCH_FILE "./resources/bench/search.txt"
#define BENCH_LOG_FILE "./resources/bench/search_log.txt"

typedef struct BenchData
{
    uint8 moves[60];
} BenchData;

void MakeBench(int nbGames, uint8 nbRandomTurn);

void Bench1Game(uint8 *moves);

void BenchSearching();

#endif