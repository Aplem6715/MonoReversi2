#ifndef EVAL_DEFINED
#define EVAL_DEFINED

#include "../const.h"
#ifdef USE_NN
#include "../ai/nnet.h"
#elif USE_REGRESSION
#include "../ai/regression.h"
#endif

//extern const uint32_t FeatMaxIndex[46];
//extern const uint8 FeatDigits[46];

extern const uint32_t FTYPE_INDEX_MAX[FEAT_TYPE_NUM];
extern const uint32_t FTYPE_DIGIT[FEAT_TYPE_NUM];

extern const score_t VALUE_TABLE[64];

typedef struct Evaluator
{
    unsigned short FeatureStates[FEAT_NUM];
    uint8 player;
#ifdef USE_NN
    NNet *net;
#elif USE_REGRESSION
    Regressor *regr;
#endif

} Evaluator;

uint16_t OpponentIndex(uint16_t idx, uint8 digit);

void EvalInit(Evaluator *eval);
void EvalDelete(Evaluator *eval);
void EvalReload(Evaluator *eval, uint64_t own, uint64_t opp, uint8 player);
void EvalUpdate(Evaluator *eval, uint8 pos, uint64_t flip);
void EvalUndo(Evaluator *eval, uint8 pos, uint64_t flip);
void EvalUpdatePass(Evaluator *eval);
//void SetWeights(Evaluator *eval, Weight *weights[NB_PHASE]);

score_t Evaluate(Evaluator *eval, uint8 nbEmpty);
score_t EvalPosTable(uint64_t own, uint64_t opp);

#endif