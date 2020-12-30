#ifndef EVAL_DEFINED
#define EVAL_DEFINED

#include "../const.h"

// パターン種類
#define NB_FEATURE_TYPES (11)
// 4回転分
#define NB_ROTATE (4)
// 全部で44パターン
#define NB_FEATURES (NB_FEATURE_TYPES * NB_ROTATE)

#define FEAT_LINE2_1 0
#define FEAT_LINE2_2 1
#define FEAT_LINE2_3 2
#define FEAT_LINE2_4 3

#define FEAT_LINE3_1 4
#define FEAT_LINE3_2 5
#define FEAT_LINE3_3 6
#define FEAT_LINE3_4 7

#define FEAT_LINE4_1 8
#define FEAT_LINE4_2 9
#define FEAT_LINE4_3 10
#define FEAT_LINE4_4 11

#define FEAT_DIAG4_1 12
#define FEAT_DIAG4_2 13
#define FEAT_DIAG4_3 14
#define FEAT_DIAG4_4 15

#define FEAT_DIAG5_1 16
#define FEAT_DIAG5_2 17
#define FEAT_DIAG5_3 18
#define FEAT_DIAG5_4 19

#define FEAT_DIAG6_1 20
#define FEAT_DIAG6_2 21
#define FEAT_DIAG6_3 22
#define FEAT_DIAG6_4 23

#define FEAT_DIAG7_1 24
#define FEAT_DIAG7_2 25
#define FEAT_DIAG7_3 26
#define FEAT_DIAG7_4 27

#define FEAT_DIAG8_1 28
#define FEAT_DIAG8_2 29

#define FEAT_EDGEX_1 30
#define FEAT_EDGEX_2 31
#define FEAT_EDGEX_3 32
#define FEAT_EDGEX_4 33

#define FEAT_CORNR_1 34
#define FEAT_CORNR_2 35
#define FEAT_CORNR_3 36
#define FEAT_CORNR_4 37

#define FEAT_BMRAN_1 38
#define FEAT_BMRAN_2 39
#define FEAT_BMRAN_3 40
#define FEAT_BMRAN_4 41
#define FEAT_NUM 42


const float VALUE_TABLE[] = {
    120, -20, 20, 5, 5, 20, -20, 120,   //
    -20, -40, -5, -5, -5, -5, -40, -20, //
    20, -5, 15, 3, 3, 15, -5, 20,       //
    5, -5, 3, 3, 3, 3, -5, 5,           //
    5, -5, 3, 3, 3, 3, -5, 5,           //
    20, -5, 15, 3, 3, 15, -5, 20,       //
    -20, -40, -5, -5, -5, -5, -40, -20, //
    120, -20, 20, 5, 5, 20, -20, 120,   //
};

typedef struct Evaluator
{
    unsigned short FeatureStates[FEAT_NUM];
    unsigned char isOwn;
} Evaluator;

void InitEval(Evaluator *eval, uint64 own, uint64 opp);
void UpdateEval(Evaluator *eval, uint8 pos, uint64 flip);
void UndoEval(Evaluator *eval, uint8 pos, uint64 flip);
void UpdateEvalPass(Evaluator *eval);

float EvalPosTable(uint64 own, uint64 opp);

#endif