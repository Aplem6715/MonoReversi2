#ifndef REGRESSION_DEFINED
#define REGRESSION_DEFINED

#include "../learning/game_record.h"
#include "ai_const.h"

typedef struct Regressor
{
    // 重み（EDGEペア統合分のサイズを減らす)
    //float weights[NB_FEAT_COMB];
    // weight[player][feat_index][pattern_pow3-shape]
    float *weight[2][FEAT_NUM];
#ifdef LEARN_MODE
    uint32_t *nbAppears[FEAT_NUM];
    float *del[FEAT_NUM];
    float beta;
#endif
} Regressor;

void InitRegr(Regressor regr[NB_PHASE]);
void DelRegr(Regressor regr[NB_PHASE]);
void RegrClearWeight(Regressor regr[NB_PHASE]);
float RegrPred(Regressor *regr, const uint16_t features[], uint8 player);

#ifdef LEARN_MODE
void RegrTrainInit(Regressor regr[NB_PHASE]);
void RegrInitBeta(Regressor regr[NB_PHASE]);
float RegrTrain(Regressor regr[NB_PHASE], vector<FeatureRecord> &featRecords, FeatureRecord *testRecords, size_t nbTests);
void RegrDecreaseBeta(Regressor regr[NB_PHASE], float mul);
#endif
void RegrSave(Regressor regr[NB_PHASE], const char *file);
void RegrLoad(Regressor regr[NB_PHASE], const char *file);

#endif