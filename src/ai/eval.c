/**
 * @file eval.c
 * @author Daichi Sato
 * @brief 盤面の評価を行う評価関数の定義
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 * 反転・対称型は学習時に統合を行っている。
 * 保存ファイルの容量は増えるが，読み込み処理の簡単化を優先した。
 * 
 * M-buroさんの論文を参考に作成。パターンなども同じパターンを使った。
 * https://skatgame.net/mburo/ps/improve.pdf
 */

#include "eval.h"
#include "ai_const.h"
#include "../bit_operation.h"
#include <assert.h>
#include <stdlib.h>
#include <math.h>

/**
 * テスト済み
 * @brief 敵味方逆のパターンインデックスを取得する
 * 
 * @param idx インデックス
 * @param digit インデックスの３進桁数
 * @return uint16 逆立場のインデックス
 */
uint16_t OpponentIndex(uint16_t idx, uint8 digit)
{
    const uint16_t oppN[] = {0, 2, 1};
    uint16_t ret = 0;
    uint8 shift;
    for (shift = 0; shift < digit; shift++)
    {
        ret += oppN[idx % 3] * POW3_LIST[shift];
        idx /= 3;
    }
    return ret;
}

typedef struct PosToFeature
{
    int nbFeature;
    struct
    {
        unsigned short feat;
        unsigned short idx;
    } feature[8];
} PosToFeature;

extern const score_t VALUE_TABLE[64] = {
    20, 4, 18, 12, 12, 18, 4, 20,
    4, 1, 6, 8, 8, 6, 1, 4,
    18, 6, 15, 10, 10, 15, 6, 18,
    12, 8, 10, 0, 0, 10, 8, 12,
    12, 8, 10, 0, 0, 10, 8, 12,
    18, 6, 15, 10, 10, 15, 6, 18,
    4, 1, 6, 8, 8, 6, 1, 4,
    20, 4, 18, 12, 12, 18, 4, 20};

// 各座標と対応するパターンとその３進インデックス
static const PosToFeature Pos2Feat[] = {
    /*A1*/ {6, {{FEAT_DIAG8_2, POW3_0}, {FEAT_EDGEX_1, POW3_1}, {FEAT_EDGEX_4, POW3_8}, {FEAT_CORNR_1, POW3_0}, {FEAT_BOX10_1, POW3_0}, {FEAT_BOX10_8, POW3_0}}},
    /*B1*/ {6, {{FEAT_LINE2_4, POW3_0}, {FEAT_DIAG7_2, POW3_0}, {FEAT_EDGEX_1, POW3_2}, {FEAT_CORNR_1, POW3_1}, {FEAT_BOX10_1, POW3_1}, {FEAT_BOX10_8, POW3_5}}},
    /*C1*/ {5, {{FEAT_LINE3_4, POW3_0}, {FEAT_DIAG6_2, POW3_0}, {FEAT_EDGEX_1, POW3_3}, {FEAT_CORNR_1, POW3_2}, {FEAT_BOX10_1, POW3_2}}},
    /*D1*/ {6, {{FEAT_LINE4_4, POW3_0}, {FEAT_DIAG4_1, POW3_0}, {FEAT_DIAG5_2, POW3_0}, {FEAT_EDGEX_1, POW3_4}, {FEAT_BOX10_1, POW3_3}, {FEAT_BOX10_2, POW3_4}}},
    /*E1*/ {6, {{FEAT_LINE4_2, POW3_0}, {FEAT_DIAG4_2, POW3_0}, {FEAT_DIAG5_1, POW3_0}, {FEAT_EDGEX_1, POW3_5}, {FEAT_BOX10_1, POW3_4}, {FEAT_BOX10_2, POW3_3}}},
    /*F1*/ {5, {{FEAT_LINE3_2, POW3_0}, {FEAT_DIAG6_1, POW3_0}, {FEAT_EDGEX_1, POW3_6}, {FEAT_CORNR_2, POW3_6}, {FEAT_BOX10_2, POW3_2}}},
    /*G1*/ {6, {{FEAT_LINE2_2, POW3_0}, {FEAT_DIAG7_1, POW3_0}, {FEAT_EDGEX_1, POW3_7}, {FEAT_CORNR_2, POW3_3}, {FEAT_BOX10_2, POW3_1}, {FEAT_BOX10_3, POW3_5}}},
    /*H1*/ {6, {{FEAT_DIAG8_1, POW3_0}, {FEAT_EDGEX_1, POW3_8}, {FEAT_EDGEX_2, POW3_1}, {FEAT_CORNR_2, POW3_0}, {FEAT_BOX10_2, POW3_0}, {FEAT_BOX10_3, POW3_0}}},
    //
    /*A2*/ {6, {{FEAT_LINE2_1, POW3_0}, {FEAT_DIAG7_4, POW3_0}, {FEAT_EDGEX_4, POW3_7}, {FEAT_CORNR_1, POW3_3}, {FEAT_BOX10_1, POW3_5}, {FEAT_BOX10_8, POW3_1}}},
    /*B2*/ {8, {{FEAT_LINE2_1, POW3_1}, {FEAT_LINE2_4, POW3_1}, {FEAT_DIAG8_2, POW3_1}, {FEAT_EDGEX_1, POW3_0}, {FEAT_EDGEX_4, POW3_9}, {FEAT_CORNR_1, POW3_4}, {FEAT_BOX10_1, POW3_6}, {FEAT_BOX10_8, POW3_6}}},
    /*C2*/ {6, {{FEAT_LINE2_1, POW3_2}, {FEAT_LINE3_4, POW3_1}, {FEAT_DIAG4_1, POW3_1}, {FEAT_DIAG7_2, POW3_1}, {FEAT_CORNR_1, POW3_5}, {FEAT_BOX10_1, POW3_7}}},
    /*D2*/ {6, {{FEAT_LINE2_1, POW3_3}, {FEAT_LINE4_4, POW3_1}, {FEAT_DIAG5_1, POW3_1}, {FEAT_DIAG6_2, POW3_1}, {FEAT_BOX10_1, POW3_8}, {FEAT_BOX10_2, POW3_9}}},
    /*E2*/ {6, {{FEAT_LINE2_1, POW3_4}, {FEAT_LINE4_2, POW3_1}, {FEAT_DIAG5_2, POW3_1}, {FEAT_DIAG6_1, POW3_1}, {FEAT_BOX10_1, POW3_9}, {FEAT_BOX10_2, POW3_8}}},
    /*F2*/ {6, {{FEAT_LINE2_1, POW3_5}, {FEAT_LINE3_2, POW3_1}, {FEAT_DIAG4_2, POW3_1}, {FEAT_DIAG7_1, POW3_1}, {FEAT_CORNR_2, POW3_7}, {FEAT_BOX10_2, POW3_7}}},
    /*G2*/ {8, {{FEAT_LINE2_1, POW3_6}, {FEAT_LINE2_2, POW3_1}, {FEAT_DIAG8_1, POW3_1}, {FEAT_EDGEX_1, POW3_9}, {FEAT_EDGEX_2, POW3_0}, {FEAT_CORNR_2, POW3_4}, {FEAT_BOX10_2, POW3_6}, {FEAT_BOX10_3, POW3_6}}},
    /*H2*/ {6, {{FEAT_LINE2_1, POW3_7}, {FEAT_DIAG7_3, POW3_0}, {FEAT_EDGEX_2, POW3_2}, {FEAT_CORNR_2, POW3_1}, {FEAT_BOX10_2, POW3_5}, {FEAT_BOX10_3, POW3_1}}},
    //
    /*A3*/ {5, {{FEAT_LINE3_1, POW3_0}, {FEAT_DIAG6_4, POW3_0}, {FEAT_EDGEX_4, POW3_6}, {FEAT_CORNR_1, POW3_6}, {FEAT_BOX10_8, POW3_2}}},
    /*B3*/ {6, {{FEAT_LINE3_1, POW3_1}, {FEAT_LINE2_4, POW3_2}, {FEAT_DIAG4_1, POW3_2}, {FEAT_DIAG7_4, POW3_1}, {FEAT_CORNR_1, POW3_7}, {FEAT_BOX10_8, POW3_7}}},
    /*C3*/ {5, {{FEAT_LINE3_1, POW3_2}, {FEAT_LINE3_4, POW3_2}, {FEAT_DIAG5_1, POW3_2}, {FEAT_DIAG8_2, POW3_2}, {FEAT_CORNR_1, POW3_8}}},
    /*D3*/ {4, {{FEAT_LINE3_1, POW3_3}, {FEAT_LINE4_4, POW3_2}, {FEAT_DIAG6_1, POW3_2}, {FEAT_DIAG7_2, POW3_2}}},
    /*E3*/ {4, {{FEAT_LINE3_1, POW3_4}, {FEAT_LINE4_2, POW3_2}, {FEAT_DIAG6_2, POW3_2}, {FEAT_DIAG7_1, POW3_2}}},
    /*F3*/ {5, {{FEAT_LINE3_1, POW3_5}, {FEAT_LINE3_2, POW3_2}, {FEAT_DIAG5_2, POW3_2}, {FEAT_DIAG8_1, POW3_2}, {FEAT_CORNR_2, POW3_8}}},
    /*G3*/ {6, {{FEAT_LINE3_1, POW3_6}, {FEAT_LINE2_2, POW3_2}, {FEAT_DIAG4_2, POW3_2}, {FEAT_DIAG7_3, POW3_1}, {FEAT_CORNR_2, POW3_5}, {FEAT_BOX10_3, POW3_7}}},
    /*H3*/ {5, {{FEAT_LINE3_1, POW3_7}, {FEAT_DIAG6_3, POW3_0}, {FEAT_EDGEX_2, POW3_3}, {FEAT_CORNR_2, POW3_2}, {FEAT_BOX10_3, POW3_2}}},
    //
    /*A4*/ {6, {{FEAT_LINE4_1, POW3_0}, {FEAT_DIAG4_1, POW3_3}, {FEAT_DIAG5_4, POW3_0}, {FEAT_EDGEX_4, POW3_5}, {FEAT_BOX10_7, POW3_4}, {FEAT_BOX10_8, POW3_3}}},
    /*B4*/ {6, {{FEAT_LINE4_1, POW3_1}, {FEAT_LINE2_4, POW3_3}, {FEAT_DIAG5_1, POW3_3}, {FEAT_DIAG6_4, POW3_1}, {FEAT_BOX10_7, POW3_9}, {FEAT_BOX10_8, POW3_8}}},
    /*C4*/ {4, {{FEAT_LINE4_1, POW3_2}, {FEAT_LINE3_4, POW3_3}, {FEAT_DIAG6_1, POW3_3}, {FEAT_DIAG7_4, POW3_2}}},
    /*D4*/ {4, {{FEAT_LINE4_1, POW3_3}, {FEAT_LINE4_4, POW3_3}, {FEAT_DIAG7_1, POW3_3}, {FEAT_DIAG8_2, POW3_3}}},
    /*E4*/ {4, {{FEAT_LINE4_1, POW3_4}, {FEAT_LINE4_2, POW3_3}, {FEAT_DIAG7_2, POW3_3}, {FEAT_DIAG8_1, POW3_3}}},
    /*F4*/ {4, {{FEAT_LINE4_1, POW3_5}, {FEAT_LINE3_2, POW3_3}, {FEAT_DIAG6_2, POW3_3}, {FEAT_DIAG7_3, POW3_2}}},
    /*G4*/ {6, {{FEAT_LINE4_1, POW3_6}, {FEAT_LINE2_2, POW3_3}, {FEAT_DIAG5_2, POW3_3}, {FEAT_DIAG6_3, POW3_1}, {FEAT_BOX10_3, POW3_8}, {FEAT_BOX10_4, POW3_9}}},
    /*H4*/ {6, {{FEAT_LINE4_1, POW3_7}, {FEAT_DIAG4_2, POW3_3}, {FEAT_DIAG5_3, POW3_0}, {FEAT_EDGEX_2, POW3_4}, {FEAT_BOX10_3, POW3_3}, {FEAT_BOX10_4, POW3_4}}},
    //
    /*A5*/ {6, {{FEAT_LINE4_3, POW3_0}, {FEAT_DIAG4_4, POW3_0}, {FEAT_DIAG5_1, POW3_4}, {FEAT_EDGEX_4, POW3_4}, {FEAT_BOX10_7, POW3_3}, {FEAT_BOX10_8, POW3_4}}},
    /*B5*/ {6, {{FEAT_LINE4_3, POW3_1}, {FEAT_LINE2_4, POW3_4}, {FEAT_DIAG5_4, POW3_1}, {FEAT_DIAG6_1, POW3_4}, {FEAT_BOX10_7, POW3_8}, {FEAT_BOX10_8, POW3_9}}},
    /*C5*/ {4, {{FEAT_LINE4_3, POW3_2}, {FEAT_LINE3_4, POW3_4}, {FEAT_DIAG6_4, POW3_2}, {FEAT_DIAG7_1, POW3_4}}},
    /*D5*/ {4, {{FEAT_LINE4_3, POW3_3}, {FEAT_LINE4_4, POW3_4}, {FEAT_DIAG7_4, POW3_3}, {FEAT_DIAG8_1, POW3_4}}},
    /*E5*/ {4, {{FEAT_LINE4_3, POW3_4}, {FEAT_LINE4_2, POW3_4}, {FEAT_DIAG7_3, POW3_3}, {FEAT_DIAG8_2, POW3_4}}},
    /*F5*/ {4, {{FEAT_LINE4_3, POW3_5}, {FEAT_LINE3_2, POW3_4}, {FEAT_DIAG6_3, POW3_2}, {FEAT_DIAG7_2, POW3_4}}},
    /*G5*/ {6, {{FEAT_LINE4_3, POW3_6}, {FEAT_LINE2_2, POW3_4}, {FEAT_DIAG5_3, POW3_1}, {FEAT_DIAG6_2, POW3_4}, {FEAT_BOX10_3, POW3_9}, {FEAT_BOX10_4, POW3_8}}},
    /*H5*/ {6, {{FEAT_LINE4_3, POW3_7}, {FEAT_DIAG4_3, POW3_0}, {FEAT_DIAG5_2, POW3_4}, {FEAT_EDGEX_2, POW3_5}, {FEAT_BOX10_3, POW3_4}, {FEAT_BOX10_4, POW3_3}}},
    //
    /*A6*/ {5, {{FEAT_LINE3_3, POW3_0}, {FEAT_DIAG6_1, POW3_5}, {FEAT_EDGEX_4, POW3_3}, {FEAT_CORNR_4, POW3_6}, {FEAT_BOX10_7, POW3_2}}},
    /*B6*/ {6, {{FEAT_LINE3_3, POW3_1}, {FEAT_LINE2_4, POW3_5}, {FEAT_DIAG4_4, POW3_1}, {FEAT_DIAG7_1, POW3_5}, {FEAT_CORNR_4, POW3_7}, {FEAT_BOX10_7, POW3_7}}},
    /*C6*/ {5, {{FEAT_LINE3_3, POW3_2}, {FEAT_LINE3_4, POW3_5}, {FEAT_DIAG5_4, POW3_2}, {FEAT_DIAG8_1, POW3_5}, {FEAT_CORNR_4, POW3_8}}},
    /*D6*/ {4, {{FEAT_LINE3_3, POW3_3}, {FEAT_LINE4_4, POW3_5}, {FEAT_DIAG6_4, POW3_3}, {FEAT_DIAG7_3, POW3_4}}},
    /*E6*/ {4, {{FEAT_LINE3_3, POW3_4}, {FEAT_LINE4_2, POW3_5}, {FEAT_DIAG6_3, POW3_3}, {FEAT_DIAG7_4, POW3_4}}},
    /*F6*/ {5, {{FEAT_LINE3_3, POW3_5}, {FEAT_LINE3_2, POW3_5}, {FEAT_DIAG5_3, POW3_2}, {FEAT_DIAG8_2, POW3_5}, {FEAT_CORNR_3, POW3_8}}},
    /*G6*/ {6, {{FEAT_LINE3_3, POW3_6}, {FEAT_LINE2_2, POW3_5}, {FEAT_DIAG4_3, POW3_1}, {FEAT_DIAG7_2, POW3_5}, {FEAT_CORNR_3, POW3_7}, {FEAT_BOX10_4, POW3_7}}},
    /*H6*/ {5, {{FEAT_LINE3_3, POW3_7}, {FEAT_DIAG6_2, POW3_5}, {FEAT_EDGEX_2, POW3_6}, {FEAT_CORNR_3, POW3_6}, {FEAT_BOX10_4, POW3_2}}},
    //
    /*A7*/ {6, {{FEAT_LINE2_3, POW3_0}, {FEAT_DIAG7_1, POW3_6}, {FEAT_EDGEX_4, POW3_2}, {FEAT_CORNR_4, POW3_3}, {FEAT_BOX10_6, POW3_5}, {FEAT_BOX10_7, POW3_1}}},
    /*B7*/ {8, {{FEAT_LINE2_3, POW3_1}, {FEAT_LINE2_4, POW3_6}, {FEAT_DIAG8_1, POW3_6}, {FEAT_EDGEX_3, POW3_9}, {FEAT_EDGEX_4, POW3_0}, {FEAT_CORNR_4, POW3_4}, {FEAT_BOX10_6, POW3_6}, {FEAT_BOX10_7, POW3_6}}},
    /*C7*/ {6, {{FEAT_LINE2_3, POW3_2}, {FEAT_LINE3_4, POW3_6}, {FEAT_DIAG4_4, POW3_2}, {FEAT_DIAG7_3, POW3_5}, {FEAT_CORNR_4, POW3_5}, {FEAT_BOX10_6, POW3_7}}},
    /*D7*/ {6, {{FEAT_LINE2_3, POW3_3}, {FEAT_LINE4_4, POW3_6}, {FEAT_DIAG5_4, POW3_3}, {FEAT_DIAG6_3, POW3_4}, {FEAT_BOX10_5, POW3_9}, {FEAT_BOX10_6, POW3_8}}},
    /*E7*/ {6, {{FEAT_LINE2_3, POW3_4}, {FEAT_LINE4_2, POW3_6}, {FEAT_DIAG5_3, POW3_3}, {FEAT_DIAG6_4, POW3_4}, {FEAT_BOX10_5, POW3_8}, {FEAT_BOX10_6, POW3_9}}},
    /*F7*/ {6, {{FEAT_LINE2_3, POW3_5}, {FEAT_LINE3_2, POW3_6}, {FEAT_DIAG4_3, POW3_2}, {FEAT_DIAG7_4, POW3_5}, {FEAT_CORNR_3, POW3_5}, {FEAT_BOX10_5, POW3_7}}},
    /*G7*/ {8, {{FEAT_LINE2_3, POW3_6}, {FEAT_LINE2_2, POW3_6}, {FEAT_DIAG8_2, POW3_6}, {FEAT_EDGEX_2, POW3_9}, {FEAT_EDGEX_3, POW3_0}, {FEAT_CORNR_3, POW3_4}, {FEAT_BOX10_4, POW3_6}, {FEAT_BOX10_5, POW3_6}}},
    /*H7*/ {6, {{FEAT_LINE2_3, POW3_7}, {FEAT_DIAG7_2, POW3_6}, {FEAT_EDGEX_2, POW3_7}, {FEAT_CORNR_3, POW3_3}, {FEAT_BOX10_4, POW3_1}, {FEAT_BOX10_5, POW3_5}}},
    //
    /*A8*/ {6, {{FEAT_DIAG8_1, POW3_7}, {FEAT_EDGEX_3, POW3_8}, {FEAT_EDGEX_4, POW3_1}, {FEAT_CORNR_4, POW3_0}, {FEAT_BOX10_6, POW3_0}, {FEAT_BOX10_7, POW3_0}}},
    /*B8*/ {6, {{FEAT_LINE2_4, POW3_7}, {FEAT_DIAG7_3, POW3_6}, {FEAT_EDGEX_3, POW3_7}, {FEAT_CORNR_4, POW3_1}, {FEAT_BOX10_6, POW3_1}, {FEAT_BOX10_7, POW3_5}}},
    /*C8*/ {5, {{FEAT_LINE3_4, POW3_7}, {FEAT_DIAG6_3, POW3_5}, {FEAT_EDGEX_3, POW3_6}, {FEAT_CORNR_4, POW3_2}, {FEAT_BOX10_6, POW3_2}}},
    /*D8*/ {6, {{FEAT_LINE4_4, POW3_7}, {FEAT_DIAG4_4, POW3_3}, {FEAT_DIAG5_3, POW3_4}, {FEAT_EDGEX_3, POW3_5}, {FEAT_BOX10_5, POW3_4}, {FEAT_BOX10_6, POW3_3}}},
    /*E8*/ {6, {{FEAT_LINE4_2, POW3_7}, {FEAT_DIAG4_3, POW3_3}, {FEAT_DIAG5_4, POW3_4}, {FEAT_EDGEX_3, POW3_4}, {FEAT_BOX10_5, POW3_3}, {FEAT_BOX10_6, POW3_4}}},
    /*F8*/ {5, {{FEAT_LINE3_2, POW3_7}, {FEAT_DIAG6_4, POW3_5}, {FEAT_EDGEX_3, POW3_3}, {FEAT_CORNR_3, POW3_2}, {FEAT_BOX10_5, POW3_2}}},
    /*G8*/ {6, {{FEAT_LINE2_2, POW3_7}, {FEAT_DIAG7_4, POW3_6}, {FEAT_EDGEX_3, POW3_2}, {FEAT_CORNR_3, POW3_1}, {FEAT_BOX10_4, POW3_5}, {FEAT_BOX10_5, POW3_1}}},
    /*H8*/ {6, {{FEAT_DIAG8_2, POW3_7}, {FEAT_EDGEX_2, POW3_8}, {FEAT_EDGEX_3, POW3_1}, {FEAT_CORNR_3, POW3_0}, {FEAT_BOX10_4, POW3_0}, {FEAT_BOX10_5, POW3_0}}},
};

// ALL 211,734
/*
extern const uint32_t FeatMaxIndex[46] = {
    POW3_8, POW3_8, POW3_8, POW3_8,     // LINE2  26244
    POW3_8, POW3_8, POW3_8, POW3_8,     // LINE3  26244
    POW3_8, POW3_8, POW3_8, POW3_8,     // LINE4  26244
    POW3_4, POW3_4, POW3_4, POW3_4,     // DIAG4    324
    POW3_5, POW3_5, POW3_5, POW3_5,     // DIAG5    972
    POW3_6, POW3_6, POW3_6, POW3_6,     // DIAG6   2916
    POW3_7, POW3_7, POW3_7, POW3_7,     // DIAG7   8748
    POW3_8, POW3_8,                     // DIAG8  13122
    POW3_10, POW3_10, POW3_10, POW3_10, // EDGEX 236196
    POW3_9, POW3_9, POW3_9, POW3_9,     // CORNR  78732
    POW3_10, POW3_10, POW3_10, POW3_10, // BOX10 236196
    POW3_10, POW3_10, POW3_10, POW3_10, // BOX10 236196
};

extern const uint8 FeatDigits[46] = {
    8, 8, 8, 8,     // LINE
    8, 8, 8, 8,     //
    8, 8, 8, 8,     //
    4, 4, 4, 4,     // DIAG
    5, 5, 5, 5,     //
    6, 6, 6, 6,     //
    7, 7, 7, 7,     //
    8, 8,           //
    10, 10, 10, 10, // EDGEX
    9, 9, 9, 9,     // CORNR
    10, 10, 10, 10, // BOX10
    10, 10, 10, 10  // BOX10
};*/

extern const uint32_t FTYPE_INDEX_MAX[FEAT_TYPE_NUM] = {
    POW3_8,  // LINE2  26244
    POW3_8,  // LINE3  26244
    POW3_8,  // LINE4  26244
    POW3_4,  // DIAG4    324
    POW3_5,  // DIAG5    972
    POW3_6,  // DIAG6   2916
    POW3_7,  // DIAG7   8748
    POW3_8,  // DIAG8  13122
    POW3_10, // EDGEX 236196
    POW3_9,  // CORNR  78732
    POW3_10, // BOX10 236196
};
/*
extern const uint8 FeatDigits[46] = {
    8,  // LINE2
    8,  // LINE3
    8,  // LINE4
    4,  // DIAG4
    5,  // DIAG5
    6,  // DIAG6
    7,  // DIAG7
    8,  // DIAG8
    10, // EDGEX
    9,  // CORNR
    10, // BOX10
};*/

extern const uint32_t FTYPE_DIGIT[FEAT_TYPE_NUM] = {
    8,  // LINE2
    8,  // LINE3
    8,  // LINE4
    4,  // DIAG4
    5,  // DIAG5
    6,  // DIAG6
    7,  // DIAG7
    8,  // DIAG8
    10, // EDGEX
    9,  // CORNR
    10, // BOX10
};

//static const uint16 DEBUG_TARGET_FEAT = 3;
static const char modelFolder[] = "resources/model/model_2003-epoch1/";
//static const char regrFolder[] = "resources/regressor/best/"; //"resources/regressor/regr_1981_Loss1473/";
//static const char regrFolder[] = "resources/regressor/regrV3_393_Loss1528/";弱かった・・・
static const char regrFolder[] = "resources/regressor/regrV4_Ascii0_Loss1577/";

void EvalInit(Evaluator *eval)
{
#ifdef USE_NN
    eval->net = (NNet *)malloc(sizeof(NNet) * NB_PHASE);
    LoadNets(eval->net, modelFolder);
#elif USE_REGRESSION
    eval->regr = (Regressor *)malloc(sizeof(Regressor) * NB_PHASE);
    InitRegr(eval->regr);
    RegrLoad(eval->regr, regrFolder);
#endif
}

void EvalDelete(Evaluator *eval)
{
#ifdef USE_NN
#elif USE_REGRESSION
    DelRegr(eval->regr);
    free(eval->regr);
#endif
}

void EvalReload(Evaluator *eval, uint64_t own, uint64_t opp, uint8 player)
{
    const PosToFeature *pos2f;
    int nbFeat;

    // 自分の手番
    eval->player = player;
    for (int ftype = 0; ftype < FEAT_NUM; ftype++)
    {
        eval->FeatureStates[ftype] = 0;
    }

    // 自分の石がある位置について
    for (uint8 pos = PosIndexFromBit(own); own; pos = NextIndex(&own))
    {
        pos2f = &(Pos2Feat[pos]);
        nbFeat = pos2f->nbFeature;
        // 関連するすべての特徴のインデックスを更新
        for (int i = 0; i < nbFeat; i++)
        {
            // 0(empty) -> 1(own)
            eval->FeatureStates[pos2f->feature[i].feat] += pos2f->feature[i].idx;
        }
    }
    // 相手の石がある位置について
    for (uint8 pos = PosIndexFromBit(opp); opp; pos = NextIndex(&opp))
    {
        pos2f = &(Pos2Feat[pos]);
        nbFeat = pos2f->nbFeature;
        // 関連するすべての特徴のインデックスを更新
        for (int i = 0; i < nbFeat; i++)
        {
            // 0(empty) -> 2(opp)
            eval->FeatureStates[pos2f->feature[i].feat] += 2 * pos2f->feature[i].idx;
        }
    }
    assert(own == 0 && opp == 0);
}

void EvalUpdate(Evaluator *eval, uint8 pos, uint64_t flip)
{
    const PosToFeature *pos2f = &(Pos2Feat[pos]);
    int nbFeat = pos2f->nbFeature;
    uint8 flipIdx;
    if (eval->player == OWN)
    {
        // 着手箇所について
        // 関連するすべての特徴のインデックスを更新
        for (int i = 0; i < nbFeat; i++)
        {
            // 0(empty) -> 1(own)
            eval->FeatureStates[pos2f->feature[i].feat] += pos2f->feature[i].idx;
            assert(eval->FeatureStates[pos2f->feature[i].feat] < FTYPE_INDEX_MAX[FeatID2Type[pos2f->feature[i].feat]]);
        }

        // 反転箇所について
        for (flipIdx = PosIndexFromBit(flip); flip; flipIdx = NextIndex(&flip))
        {
            pos2f = &(Pos2Feat[flipIdx]);
            nbFeat = pos2f->nbFeature;
            // 関連するすべての特徴のインデックスを更新
            for (int i = 0; i < nbFeat; i++)
            {
                // 2(opp) -> 1(own)
                eval->FeatureStates[pos2f->feature[i].feat] -= pos2f->feature[i].idx;
                assert(eval->FeatureStates[pos2f->feature[i].feat] < FTYPE_INDEX_MAX[FeatID2Type[pos2f->feature[i].feat]]);
            }
        }
    }
    else
    {
        // 着手箇所について
        // 関連するすべての特徴のインデックスを戻す
        for (int i = 0; i < nbFeat; i++)
        {
            // 0(empty) -> 2(opp)
            eval->FeatureStates[pos2f->feature[i].feat] += 2 * pos2f->feature[i].idx;
            assert(eval->FeatureStates[pos2f->feature[i].feat] < FTYPE_INDEX_MAX[FeatID2Type[pos2f->feature[i].feat]]);
        }

        // 反転箇所について
        for (flipIdx = PosIndexFromBit(flip); flip; flipIdx = NextIndex(&flip))
        {
            pos2f = &(Pos2Feat[flipIdx]);
            nbFeat = pos2f->nbFeature;
            // 関連するすべての特徴のインデックスを戻す
            for (int i = 0; i < nbFeat; i++)
            {
                // 1(own) -> 2(opp)
                eval->FeatureStates[pos2f->feature[i].feat] += pos2f->feature[i].idx;
                assert(eval->FeatureStates[pos2f->feature[i].feat] < FTYPE_INDEX_MAX[FeatID2Type[pos2f->feature[i].feat]]);
            }
        }
    }
    eval->player ^= 1;
}

void EvalUndo(Evaluator *eval, uint8 pos, uint64_t flip)
{
    const PosToFeature *pos2f = &(Pos2Feat[pos]);
    int nbFeat = pos2f->nbFeature;
    int i;
    uint8 flipIdx;

    eval->player ^= 1;
    if (eval->player == OWN)
    {
        // 着手箇所について
        // 関連するすべての特徴のインデックスを戻す
        for (i = 0; i < nbFeat; i++)
        {
            // 1(own) -> 0(empty)
            eval->FeatureStates[pos2f->feature[i].feat] -= pos2f->feature[i].idx;
            //assert(eval->FeatureStates[pos2f->feature[i].feat] < FeatMaxIndex[pos2f->feature[i].feat]);
        }

        // 反転箇所について
        for (flipIdx = PosIndexFromBit(flip); flip; flipIdx = NextIndex(&flip))
        {
            pos2f = &(Pos2Feat[flipIdx]);
            nbFeat = pos2f->nbFeature;
            // 関連するすべての特徴のインデックスを戻す
            for (i = 0; i < nbFeat; i++)
            {
                // 1(own) -> 2(opp)
                eval->FeatureStates[pos2f->feature[i].feat] += pos2f->feature[i].idx;
                //assert(eval->FeatureStates[pos2f->feature[i].feat] < FeatMaxIndex[pos2f->feature[i].feat]);
            }
        }
    }
    else
    {
        // 着手箇所について
        // 関連するすべての特徴のインデックスを戻す
        for (i = 0; i < nbFeat; i++)
        {
            // 2(opp) -> 0(empty)
            eval->FeatureStates[pos2f->feature[i].feat] -= 2 * pos2f->feature[i].idx;
            //assert(eval->FeatureStates[pos2f->feature[i].feat] < FeatMaxIndex[pos2f->feature[i].feat]);
        }

        // 反転箇所について
        for (flipIdx = PosIndexFromBit(flip); flip; flipIdx = NextIndex(&flip))
        {
            pos2f = &(Pos2Feat[flipIdx]);
            nbFeat = pos2f->nbFeature;
            // 関連するすべての特徴のインデックスを戻す
            for (i = 0; i < nbFeat; i++)
            {
                // 2(opp) -> 1(own)
                eval->FeatureStates[pos2f->feature[i].feat] -= pos2f->feature[i].idx;
                //assert(eval->FeatureStates[pos2f->feature[i].feat] < FeatMaxIndex[pos2f->feature[i].feat]);
            }
        }
    }
}

void EvalUpdatePass(Evaluator *eval)
{
    eval->player ^= 1;
}

score_t Evaluate(Evaluator *eval, uint8 nbEmpty)
{
    double scoref;
    score_t score;
#ifdef USE_NN
    if (eval->player)
    {
        score = Predict(&eval->net[PHASE(eval->nbEmpty)], eval->FeatureStates);
    }
    else
    {
        score = -Predict(&eval->net[PHASE(eval->nbEmpty)], eval->FeatureStates);
    }
#elif USE_REGRESSION
    scoref = RegrPred(&eval->regr[PHASE(nbEmpty)], eval->FeatureStates, eval->player);
    score = (score_t)roundl(scoref * STONE_VALUE);
#endif

    // 最小値以上，最大値以下に
    score = MAX(score, EVAL_MIN);
    score = MIN(score, EVAL_MAX);
    return score;
}

score_t EvalPosTable(uint64_t own, uint64_t opp)
{
    int i = 0;
    score_t score = 0;
    for (i = 0; i < BOARD_SIZE * BOARD_SIZE; i++)
    {
        score += ((own >> i) & 1) * VALUE_TABLE[i];
        score -= ((opp >> i) & 1) * VALUE_TABLE[i];
    }
    return score;
}
