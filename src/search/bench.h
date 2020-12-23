#ifndef BENCH_DEFINED
#define BENCH_DEFINED

#include <vector>
#include "search.h"
#include "../const.h"

#define SEARCH_BENCH_FILE "./resources/bench/search.txt"
#define BENCH_LOG_FILE "./resources/bench/search_log.txt"

void MakeBench(int nbGames, uint8 nbRandomTurn);

void Bench1Game(SearchTree &tree, std::vector<uint8> moves, int nbPut, std::ofstream &logfile);

void BenchSearching(std::vector<unsigned char> depths);

#endif