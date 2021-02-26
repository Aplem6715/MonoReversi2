#ifndef REGRESSION_DEFINED
#define REGRESSION_DEFINED

#include "ai_const.h"

typedef struct Regressor
{
    // 重み（EDGEペア統合分のサイズを減らす)
    //float weights[NB_FEAT_COMB];
    // weight[player][feat_index][pattern_pow3-shape]
    double *weight[2][FEAT_NUM];
#ifdef LEARN_MODE
    uint32_t *nbAppears[FEAT_NUM];
    double *del[FEAT_NUM];
    double beta;
#endif
} Regressor;

void InitRegr(Regressor regr[NB_PHASE]);
void DelRegr(Regressor regr[NB_PHASE]);
void RegrCopyWeight(Regressor src[NB_PHASE], Regressor dst[NB_PHASE]);
void RegrClearWeight(Regressor regr[NB_PHASE]);
void RegrApplyWeightToOpp(Regressor *regr);
double RegrPred(Regressor *regr, const uint16_t features[], uint8 player);

void RegrSave(Regressor regr[NB_PHASE], const char *file);
void RegrLoad(Regressor regr[NB_PHASE], const char *file);

#endif