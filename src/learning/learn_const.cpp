#include "learn_const.hpp"

#include <random>
#include <string>
using namespace std;

// 初期ランダム手数
const uint8 TRAIN_RANDOM_TURNS = 6;
// ランダム行動割合（１試合に1回くらい）
const double TRAIN_RANDOM_RATIO = 1.0 / 60.0;
// 次善手割合(PVS探索-orderingで次善手が探索されない事が多々ある→かなり高めの設定)
const double TRAIN_SECOND_RATIO = 4.0 / 60.0;

// 学習結果確認の対戦回数（１色で50試合＝全体で100）
const int TRAIN_NB_VERSUS = 50;
const uint8 VERSUS_RANDOM_TURNS = 6;

const int nbTrainCycles = 4096;
#ifdef USE_NN
const string modelFolder = "resources/model/";
const string modelName = "model_";
const int nbGameOneCycle = 512; //1024;
#elif USE_REGRESSION
const string modelFolder = "resources/regressor/";
const string modelName = "regrV4_";
const int nbGameOneCycle = 256;
#endif
const string selfPlayLogFileName = "log/self_play_V4.log";
const string recordLearnLogFileName = "log/ascii_learn.log";
const string testRecordDir = "./resources/record/WTH_7789/WTH_1982.wtb";
const unsigned int nbTest = 100;

random_device rnd;
mt19937 mt(rnd());
uniform_real_distribution<double> rnd_prob01(0.0, 1.0);