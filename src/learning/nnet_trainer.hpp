#if !defined(_NNET_TRAINER_H_)
#define _NNET_TRAINER_H_

#include "game_record.hpp"

extern "C"
{
#include "../ai/nnet.h"
}

void InitWeight(NNet *net);
void DecreaseNNlr(NNet *net);
float TrainNN(NNet *net, FeatureRecord *featRecords, FeatureRecord *testRecords, size_t nbRecords, size_t nbTests);

#endif // _NNET_TRAINER_H_
