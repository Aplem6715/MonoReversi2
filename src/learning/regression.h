#ifndef REGRESSION_DEFINED
#define REGRESSION_DEFINED

#include "game_record.h"
#include "../ai/ai_const.h"

#define FEAT_NB_COMBINATION 159246
#define NB_PHASE 15

#define REGR_NB_FEAT_COMB (FEAT_NB_COMBINATION - POW3_5 * 8 + POW3_10 * 4)

typedef struct Regressor
{
    // 重み（EDGEペア統合分のサイズを減らす)
    float weights[REGR_NB_FEAT_COMB];
#ifdef LEARN_MODE
    uint32 nbAppear[REGR_NB_FEAT_COMB];
    float delta[REGR_NB_FEAT_COMB];
    float beta;
#endif
} Regressor;

void InitRegrBeta(Regressor regr[NB_PHASE]);
void InitRegressor(Regressor regr[NB_PHASE]);
float PredRegressor(Regressor *regr, const uint16 features[]);

#ifdef LEARN_MODE
float TrainRegressor(Regressor regr[NB_PHASE], FeatureRecord *featRecords, FeatureRecord *testRecords, size_t nbRecords, size_t nbTests);
#endif
void SaveRegressor(Regressor regr[NB_PHASE], const char *file);
void LoadRegressor(Regressor regr[NB_PHASE], const char *file);

#endif