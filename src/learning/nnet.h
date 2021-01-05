#ifndef NNET_DEFINED
#define NNET_DEFINED

#include "game_record.h"

#define FEAT_NB_COMBINATION 159246
#define VALUE_HIDDEN_UNITS1 32
#define VALUE_HIDDEN_UNITS2 16

#define NB_LAYERS 3
#define NB_PHASE 15
/*
typedef struct Connection
{
    float weight;
} Connection;*/

#ifdef LEARN_MODE
typedef struct UnitState
{
    float sumIn;
    float delta;
} UnitState;
#endif

typedef struct NNet
{
    float c1[FEAT_NB_COMBINATION + 1][VALUE_HIDDEN_UNITS1];
    float c2[VALUE_HIDDEN_UNITS1 + 1][VALUE_HIDDEN_UNITS2];
    float c3[VALUE_HIDDEN_UNITS2 + 1][1];

    float out1[VALUE_HIDDEN_UNITS1];
    float out2[VALUE_HIDDEN_UNITS2];
    float out3[1];

#ifdef LEARN_MODE
    float dw1[FEAT_NB_COMBINATION + 1][VALUE_HIDDEN_UNITS1];
    float dw2[VALUE_HIDDEN_UNITS1 + 1][VALUE_HIDDEN_UNITS2];
    float dw3[VALUE_HIDDEN_UNITS2 + 1][1];

    UnitState state1[VALUE_HIDDEN_UNITS1];
    UnitState state2[VALUE_HIDDEN_UNITS2];
    UnitState state3[1];
#endif
} NNet;

float Predict(NNet *net, const uint16 features[]);

#ifdef LEARN_MODE
void InitWeight(NNet net[NB_PHASE]);
void Train(NNet net[NB_PHASE], FeatureRecord *gameRecords, size_t nbRecords);
#endif

void SaveNets(NNet net[NB_PHASE], const char *file);
void LoadNets(NNet net[NB_PHASE], const char *file);

#endif