#ifndef REGRESSION_DEFINED
#define REGRESSION_DEFINED

#include "game_record.h"
#include "ai_const.h"

#define FEAT_NB_COMBINATION 159246
#define NB_PHASE 15

#define REGR_NB_FEAT_COMB (FEAT_NB_COMBINATION - POW3_5 * 4)

typedef struct Regressor
{
    // 重み（EDGEペア統合分のサイズを減らす)
    float weights[REGR_NB_FEAT_COMB];
#ifdef LEARN_MODE
    uint32 nbAppear[REGR_NB_FEAT_COMB];
    float delta[REGR_NB_FEAT_COMB];
#endif
} Regressor;

void InitRegressor();
float PredRegressor(Regressor *regr, const uint16 features[]);
void TrainRegressor(Regressor regr[NB_PHASE], FeatureRecord *featRecord, size_t nbRecord);
void SaveRegressor(Regressor regr[NB_PHASE], const char *file);
void LoadRegressor(Regressor regr[NB_PHASE], const char *file);

#endif