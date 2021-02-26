#ifndef AI_CONST_DEFINED
#define AI_CONST_DEFINED

#include "../const.h"

// Phase関連
#define NB_PHASE 15
#define NB_PUT_1PHASE 4
#define PHASE(nbEmpty) (nbEmpty / NB_PUT_1PHASE)

#define NB_FEAT_COMB 892134
#define TYPE_NB_MAX 167265

// 3累乗
#define POW0_0 0
#define POW3_0 1
#define POW3_1 3
#define POW3_2 9
#define POW3_3 27
#define POW3_4 81
#define POW3_5 243
#define POW3_6 729
#define POW3_7 2187
#define POW3_8 6561
#define POW3_9 19683
#define POW3_10 59049
#define POW3_11 177147

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
#define FEAT_TYPE_BOX10 10
#define FEAT_TYPE_NUM 11

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
#define FEAT_BOX10_1 38
#define FEAT_BOX10_2 39
#define FEAT_BOX10_3 40
#define FEAT_BOX10_4 41
#define FEAT_BOX10_5 42
#define FEAT_BOX10_6 43
#define FEAT_BOX10_7 44
#define FEAT_BOX10_8 45

#define FEAT_NUM 46

#define WIN_BONUS 0

extern const uint16_t POW3_LIST[];
extern const uint8 FeatID2Type[FEAT_NUM];

#endif