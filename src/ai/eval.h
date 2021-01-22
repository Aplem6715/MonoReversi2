#ifndef EVAL_DEFINED
#define EVAL_DEFINED

#include "../const.h"
#ifdef USE_NN
#include "../ai/nnet.h"
#elif USE_REGRESSION
#include "../ai/regression.h"
#endif

#define FEAT_TYPE_LINE2 0
#define FEAT_TYPE_LINE3 1
#define FEAT_TYPE_LINE4 2
#define FEAT_TYPE_DIAG4 3
#define FEAT_TYPE_DIAG5 4
#define FEAT_TYPE_DIAG6 5
#define FEAT_TYPE_DIAG7 6
#define FEAT_TYPE_DIAG8 7
#define FEAT_TYPE_EDGEX 8
#define FEAT_TYPE_CORNR 9
#define FEAT_TYPE_BMRAN 10

// 3^9 x 4 = 78,732
#define FEAT_LINE2_1 0 //3^9
#define FEAT_LINE2_2 1 //3^9
#define FEAT_LINE2_3 2 //3^9
#define FEAT_LINE2_4 3 //3^9

// 3^9 x 4 = 78,732
#define FEAT_LINE3_1 4 //3^9
#define FEAT_LINE3_2 5 //3^9
#define FEAT_LINE3_3 6 //3^9
#define FEAT_LINE3_4 7 //3^9

// 3^9 x 4 = 78,732
#define FEAT_LINE4_1 8  //3^9
#define FEAT_LINE4_2 9  //3^9
#define FEAT_LINE4_3 10 //3^9
#define FEAT_LINE4_4 11 //3^9

// 3^5 x 4 = 972
#define FEAT_DIAG4_1 12 //3^5
#define FEAT_DIAG4_2 13 //3^5
#define FEAT_DIAG4_3 14 //3^5
#define FEAT_DIAG4_4 15 //3^5

// 3^6 x 4 = 2,916
#define FEAT_DIAG5_1 16 //3^6
#define FEAT_DIAG5_2 17 //3^6
#define FEAT_DIAG5_3 18 //3^6
#define FEAT_DIAG5_4 19 //3^6

// 3^7 x 4 = 8,748
#define FEAT_DIAG6_1 20 //3^6
#define FEAT_DIAG6_2 21
#define FEAT_DIAG6_3 22
#define FEAT_DIAG6_4 23

// 3^8 x 4 = 26,244
#define FEAT_DIAG7_1 24
#define FEAT_DIAG7_2 25
#define FEAT_DIAG7_3 26
#define FEAT_DIAG7_4 27

// 3^9 x 2 = 39,366
#define FEAT_DIAG8_1 28
#define FEAT_DIAG8_2 29

// 3^10 x 4 = 708,588
#define FEAT_EDGEX_1 30
#define FEAT_EDGEX_2 31
#define FEAT_EDGEX_3 32
#define FEAT_EDGEX_4 33

// 3^9 x 4 = 236,196
#define FEAT_CORNR_1 34
#define FEAT_CORNR_2 35
#define FEAT_CORNR_3 36
#define FEAT_CORNR_4 37

// 3^8 x 4 = 708,588
#define FEAT_BMRAN_1 38
#define FEAT_BMRAN_2 39
#define FEAT_BMRAN_3 40
#define FEAT_BMRAN_4 41

extern const uint32_t FeatMaxIndex[];
extern const uint8 FeatDigits[];

const score_t VALUE_TABLE[] = {
    20, 4, 18, 12, 12, 18, 4, 20,
    4, 1, 6, 8, 8, 6, 1, 4,
    18, 6, 15, 10, 10, 15, 6, 18,
    12, 8, 10, 0, 0, 10, 8, 12,
    12, 8, 10, 0, 0, 10, 8, 12,
    18, 6, 15, 10, 10, 15, 6, 18,
    4, 1, 6, 8, 8, 6, 1, 4,
    20, 4, 18, 12, 12, 18, 4, 20};

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

void InitEval(Evaluator *eval);
void DeleteEval(Evaluator *eval);
void ReloadEval(Evaluator *eval, uint64_t own, uint64_t opp, uint8 player);
void UpdateEval(Evaluator *eval, uint8 pos, uint64_t flip);
void UndoEval(Evaluator *eval, uint8 pos, uint64_t flip);
void UpdateEvalPass(Evaluator *eval);
//void SetWeights(Evaluator *eval, Weight *weights[NB_PHASE]);

score_t EvalNNet(Evaluator *eval, uint8 nbEmpty);
score_t EvalPosTable(uint64_t own, uint64_t opp);

#endif