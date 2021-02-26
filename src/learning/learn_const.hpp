
#if !defined(_LEARN_CONST_H_)
#define _LEARN_CONST_H_

extern "C"
{
#include "../const.h"
}

#include <random>
#include <string>

using namespace std;

// 初期ランダム手数
extern const uint8 TRAIN_RANDOM_TURNS;
// ランダム行動割合（１試合に1回くらい）
extern const double TRAIN_RANDOM_RATIO;
// 次善手割合(PVS探索-orderingで次善手が探索されない事が多々ある→かなり高めの設定)
extern const double TRAIN_SECOND_RATIO;

// 学習結果確認の対戦回数（１色で50試合＝全体で100）
extern const int TRAIN_NB_VERSUS;
extern const uint8 VERSUS_RANDOM_TURNS;

extern const int nbTrainCycles;
#ifdef USE_NN
extern const string modelFolder;
extern const string modelName;
extern const int nbGameOneCycle; //1024;
#elif USE_REGRESSION
extern const string modelFolder;
extern const string modelName;
extern const int nbGameOneCycle;
#endif
extern const string selfPlayLogFileName;
extern const string recordLearnLogFileName;
extern const string testRecordDir;
extern const unsigned int nbTest;

extern mt19937 mt;
extern uniform_real_distribution<double> rnd_prob01;

#endif // _LEARN_CONST_H_
