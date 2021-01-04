#include "regression.h"
#include "eval.h"
#include <stdlib.h>

void InitRegressor(Regressor regr[NB_PHASE])
{
    int phase, i;
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        for (i = 0; i < FEAT_NB_COMBINATION; i++)
        {
            regr[phase].weights[i] = 0;
        }
    }
}

float PredRegressor(Regressor *regr, const uint16 features[])
{
    int featIdx;
    float score = 0;
    uint32 shift = 0;
    for (featIdx = 0; featIdx < FEAT_EDGEX_1; featIdx++)
    {
        score += regr->weights[features[featIdx]];
        shift += FeatMaxIndex[featIdx];
    }
    // EDGEは1,2 3,4 5,6 7,8を統合する
    // (NNでは位置を学習・統合できるが，線形回帰は隣り合ったパターンを認識できないため)
    for (; featIdx < FEAT_EDGEX_8; featIdx += 2)
    {
        score += regr->weights[features[featIdx] * POW3_5 + features[featIdx + 1]];
        shift += POW3_5;
        shift += POW3_5;
    }
    for (; featIdx < FEAT_NUM; featIdx++)
    {
        score += regr->weights[features[featIdx]];
        shift += FeatMaxIndex[featIdx];
    }
    return score;
}

void IntegrateRegrWeight();
void TrainRegressor(Regressor regr[NB_PHASE], FeatureRecord *featRecord, size_t nbRecord);
void SaveRegressor(Regressor regr[NB_PHASE], const char *file);
void LoadRegressor(Regressor regr[NB_PHASE], const char *file);