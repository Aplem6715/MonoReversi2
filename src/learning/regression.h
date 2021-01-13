#ifndef REGRESSION_DEFINED
#define REGRESSION_DEFINED

#include "game_record.h"
#include "../ai/ai_const.h"

// 連結EDGE分-4

typedef struct Regressor
{
    // 重み（EDGEペア統合分のサイズを減らす)
    //float weights[NB_FEAT_COMB];
    // weight[player][feat_index][pattern_pow3-shape]
    float *weight[2][FEAT_NUM];
#ifdef LEARN_MODE
    uint32 *nbAppears[FEAT_NUM];
    float *del[FEAT_NUM];
    float beta;
#endif
} Regressor;

void InitRegr(Regressor regr[NB_PHASE]);
void DelRegr(Regressor regr[NB_PHASE]);
void ClearRegressorWeight(Regressor regr[NB_PHASE]);
float PredRegressor(Regressor *regr, const uint16 features[], uint8 player);

#ifdef LEARN_MODE
void InitRegrTrain(Regressor regr[NB_PHASE]);
void InitRegrBeta(Regressor regr[NB_PHASE]);
float TrainRegressor(Regressor regr[NB_PHASE], vector<FeatureRecord> &featRecords, FeatureRecord *testRecords, size_t nbTests);
void DecreaseRegrBeta(Regressor regr[NB_PHASE], float mul);
#endif
void SaveRegressor(Regressor regr[NB_PHASE], const char *file);
void LoadRegressor(Regressor regr[NB_PHASE], const char *file);

#endif