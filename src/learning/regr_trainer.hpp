#if !defined(_REGR_TRAINER_H_)
#define _REGR_TRAINER_H_

struct Regressor;
#include <vector>
#include "../learning/game_record.hpp"

void RegrTrainInit(Regressor regr[NB_PHASE]);
void RegrInitBeta(Regressor regr[NB_PHASE]);
float RegrTrain(Regressor regr[NB_PHASE], vector<FeatureRecord> &featRecords, FeatureRecord *testRecords, size_t nbTests);
void RegrDecreaseBeta(Regressor regr[NB_PHASE], float mul);

#endif // _REGR_TRAINER_H_
