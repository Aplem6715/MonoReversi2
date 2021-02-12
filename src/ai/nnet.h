#ifndef NNET_DEFINED
#define NNET_DEFINED

#include "ai_const.h"

#define VALUE_HIDDEN_UNITS1 32
#define VALUE_HIDDEN_UNITS2 1

#define NB_LAYERS 3
#define NB_PHASE 15

#ifdef LEARN_MODE
typedef struct UnitState
{
    float sumIn;
    float delta;
} UnitState;
#endif

typedef struct NNet
{
    float c1[NB_FEAT_COMB + 1][VALUE_HIDDEN_UNITS1];
    float c2[VALUE_HIDDEN_UNITS1 + 1][VALUE_HIDDEN_UNITS2];
    float c3[VALUE_HIDDEN_UNITS2 + 1][1];

    float out1[VALUE_HIDDEN_UNITS1];
    float out2[VALUE_HIDDEN_UNITS2];
    float out3[1];

#ifdef LEARN_MODE
    float dw1[NB_FEAT_COMB + 1][VALUE_HIDDEN_UNITS1];
    float dw2[VALUE_HIDDEN_UNITS1 + 1][VALUE_HIDDEN_UNITS2];
    float dw3[VALUE_HIDDEN_UNITS2 + 1][1];

    UnitState state1[VALUE_HIDDEN_UNITS1];
    UnitState state2[VALUE_HIDDEN_UNITS2];
    UnitState state3[1];

    float lr;
#endif
} NNet;

float Predict(NNet *net, const uint16_t features[]);

float forward(NNet *net, const uint16_t features[FEAT_NUM], uint8 isTrain);
void SaveNets(NNet *net, const char *file);
void LoadNets(NNet *net, const char *file);

#endif