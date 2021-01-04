#ifndef NNET_DEFINED
#define NNET_DEFINED

#include "game_record.h"

//#define FEAT_NB_COMBINATION 1967814
// 32-16: 1ネット約120[MB]

#define FEAT_NB_COMBINATION 163134
#define VALUE_HIDDEN_UNITS1 32
#define VALUE_HIDDEN_UNITS2 16

#define NB_LAYERS 3
#define NB_PHASE 4
/*
typedef struct Connection
{
    float weight;
} Connection;*/

#ifdef LEARN_MODE
typedef struct UnitState
{
    float dw_sum;
    float sum;
    float delta;
} UnitState;
#endif

typedef struct NNet
{
    float c1[FEAT_NB_COMBINATION + 1][VALUE_HIDDEN_UNITS1];
    float c2[VALUE_HIDDEN_UNITS1 + 1][VALUE_HIDDEN_UNITS2];
    float c3[VALUE_HIDDEN_UNITS2 + 1][1];

#ifdef LEARN_MODE
    UnitState state1[FEAT_NB_COMBINATION + 1][VALUE_HIDDEN_UNITS1];
    UnitState state2[VALUE_HIDDEN_UNITS1 + 1][VALUE_HIDDEN_UNITS2];
    UnitState state3[VALUE_HIDDEN_UNITS2 + 1][1];
#endif
} NNet;

void InitWeight(NNet net[NB_PHASE]);
float Predict(NNet *net, const uint16 features[]);
void Train(NNet net[NB_PHASE], FeatureRecord *gameRecords, size_t nbRecords);
void SaveNets(NNet net[NB_PHASE], const char *file);
void LoadNets(NNet net[NB_PHASE], const char *file);

#endif