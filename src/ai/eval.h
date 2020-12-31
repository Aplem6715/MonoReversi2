#ifndef EVAL_DEFINED
#define EVAL_DEFINED

#include "../const.h"

// パターン種類
#define NB_FEATURE_TYPES (11)
// 4回転分
#define NB_ROTATE (4)
// 全部で44パターン
#define NB_FEATURES (NB_FEATURE_TYPES * NB_ROTATE)

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

// 3^11 x 4 = 708,588
#define FEAT_EDGEX_1 30
#define FEAT_EDGEX_2 31
#define FEAT_EDGEX_3 32
#define FEAT_EDGEX_4 33

// 3^10 x 4 = 236,196
#define FEAT_CORNR_1 34
#define FEAT_CORNR_2 35
#define FEAT_CORNR_3 36
#define FEAT_CORNR_4 37

// 3^11 x 4 = 708,588
#define FEAT_BMRAN_1 38
#define FEAT_BMRAN_2 39
#define FEAT_BMRAN_3 40
#define FEAT_BMRAN_4 41
#define FEAT_NUM 42

#define FEAT_NB_COMBINATION 1967814

extern const uint32 FeatMaxIndex[];

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