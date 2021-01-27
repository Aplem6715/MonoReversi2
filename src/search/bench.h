#ifndef BENCH_DEFINED
#define BENCH_DEFINED

#include <vector>
#include "search.h"
#include "../const.h"

#define SEARCH_BENCH_FILE "./resources/bench/accurate_test.txt"
#define BENCH_LOG_DIR "./resources/bench/"

// ベンチマーク用のデータを作成（ランダム着手棋譜）
void MakeBench(int nbGames, uint8 nbRandomTurn);

// 1ゲームのベンチマークを実行
void Bench1Game(SearchTree &tree, std::vector<uint8> moves, int nbPut, std::ofstream &logfile);

// 指定深度でベンチマークの実行
void BenchSearching(vector<unsigned char> depths, unsigned char useHash, unsigned char hashDepth, unsigned char orderDepth, unsigned char pvsDepth);

#endif
