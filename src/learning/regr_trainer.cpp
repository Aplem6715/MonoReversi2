/**
 * @file regr_trainer.cpp
 * @author Daichi Sato
 * @brief 重線形回帰を用いたパターンの重み付け(学習時のみのコード)
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 * 反転・対照型パターンも同時に重み更新を行う。
 * 試合進行(フェーズと呼ぶ)によって評価を変える。
 * 学習時には隣接するフェーズからも学習を行い，急な評価の変化を防ぐ
 * ランダムサンプリング・バッチ学習
 * 
 */

#include "regr_trainer.hpp"

extern "C"
{
#include "../board.h"
#include "../ai/regression.h"
#include "../ai/eval.h"
}

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#ifdef LEARN_MODE

#define BATCH_SIZE 128
static const double BETA_INIT = 0.0025f;

/*
static const uint32_t FeatTypeMaxIndex[] = {
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
    POW3_10, // BOX10  26244
};*/

static const uint16_t FeatTypeNbRots[] = {
    4, // LINE2
    4, // LINE3
    4, // LINE4
    4, // DIAG4
    4, // DIAG5
    4, // DIAG6
    4, // DIAG7
    2, // DIAG8
    4, // EDGEX
    4, // CORNR
    8, // BOX10
};

static uint16_t SAME_INDEX_8[POW3_8];
static uint16_t SAME_INDEX_7[POW3_7];
static uint16_t SAME_INDEX_6[POW3_6];
static uint16_t SAME_INDEX_5[POW3_5];
static uint16_t SAME_INDEX_4[POW3_4];
static uint16_t SAME_INDEX_EDGE[POW3_10];
static uint16_t SAME_INDEX_CORNR[POW3_9];
//static uint16_t SAME_INDEX_BOX10[POW3_10]; 対照型ない

void RegrInitBeta(Regressor regr[NB_PHASE])
{
    int phase;
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        regr[phase].beta = BETA_INIT;
    }
}

void RegrDecreaseBeta(Regressor regr[NB_PHASE], double mul)
{
    int phase;
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        regr[phase].beta *= mul;
    }
}

// 対象型，同種類も含めたウェイトを更新する
void UpdateRegrWeights(Regressor *regr)
{
    int ftype;
    double alpha;
    uint16_t idx;
    int32_t sameIdx;

    // タイプごとの対象型インデックスへの参照を配列として持っておく
    uint16_t *FeatTypeSames[FEAT_TYPE_NUM - 1] = {
        SAME_INDEX_8,    // LINE2
        SAME_INDEX_8,    // LINE3
        SAME_INDEX_8,    // LINE4
        SAME_INDEX_4,    // DIAG4
        SAME_INDEX_5,    // DIAG5
        SAME_INDEX_6,    // DIAG6
        SAME_INDEX_7,    // DIAG7
        SAME_INDEX_8,    // DIAG8
        SAME_INDEX_EDGE, //
        SAME_INDEX_CORNR //
    };

    // 各パターンタイプについてループ
    for (ftype = 0; ftype < FEAT_TYPE_NUM; ftype++)
    {
        // 0~そのパターンタイプが取りうるインデックスの最大値までループ
        for (idx = 0; idx < FTYPE_INDEX_MAX[ftype]; idx++)
        {
            double nbAppearSum = regr->nbAppears[ftype][idx];
            double deltaSum = regr->delta[ftype][idx];

            if (nbAppearSum < 1)
                continue;

            if (ftype == FEAT_TYPE_BOX10)
            {
                sameIdx = -1;
            }
            else
            {
                sameIdx = FeatTypeSames[ftype][idx];
                nbAppearSum += regr->nbAppears[ftype][sameIdx];
                deltaSum += regr->delta[ftype][sameIdx];
            }

            alpha = fmin(regr->beta / 10.0f, regr->beta / nbAppearSum);
            // ウェイト調整
            regr->weight[0][ftype][idx] += alpha * deltaSum;
            // 多重調整防止
            regr->nbAppears[ftype][idx] = 0;

            // 対象型があれば調整
            if (sameIdx >= 0 && sameIdx != idx)
            {
                regr->weight[0][ftype][sameIdx] += alpha * deltaSum;
                // 多重調整防止
                regr->nbAppears[ftype][sameIdx] = 0;
            }
        }
    }
}

void ResetRegrState(Regressor *regr)
{
    uint16_t ftype, i;
    for (ftype = 0; ftype < FEAT_TYPE_NUM; ftype++)
    {
        for (i = 0; i < FTYPE_INDEX_MAX[ftype]; i++)
        {
            regr->nbAppears[ftype][i] = 0;
            regr->delta[ftype][i] = 0;
        }
    }
}

void CalcWeightDelta(Regressor *regr, const uint16_t features[FEAT_NUM], double error)
{
    int feat;

    for (feat = 0; feat < FEAT_NUM; feat++)
    {
        uint8 ftype = FeatID2Type[feat];
        regr->nbAppears[ftype][features[feat]]++;
        regr->delta[ftype][features[feat]] += error;
        assert(regr->nbAppears[ftype][features[feat]] < 4294967296);
    }
}

void TreinRegrBatch(Regressor *regr, FeatureRecord *inputs[BATCH_SIZE], int inputSize)
{
    int i;
    static bool debug = false;
    double teacher, output, loss = 0;

    ResetRegrState(regr);
    for (i = 0; i < inputSize; i++)
    {
        teacher = (double)inputs[i]->stoneDiff;
        output = RegrPred(regr, inputs[i]->featStats[OWN], OWN);
        CalcWeightDelta(regr, inputs[i]->featStats[OWN], teacher - output);

        /*
        // 反転パターンも学習
        output = RegrPred(regr, inputs[i]->featStats[OPP], OWN); // ※featの方を反転しているのでplayerは0に
        CalcWeightDelta(regr, inputs[i]->featStats[OPP], (-teacher) - output);
        */
        if (debug)
        {
            //Board::Draw(inputs[i]->own, inputs[i]->opp, 0);
            printf("Color: %d,  Score: %f, Pred: %f\n", inputs[i]->color, teacher, output);
        }
    }
    UpdateRegrWeights(regr);
    RegrApplyWeightToOpp(regr);
}

void RegrTrainInit(Regressor regr[NB_PHASE])
{
    static uint16_t *SIMPLE_SAMES[] = {
        SAME_INDEX_4,
        SAME_INDEX_5,
        SAME_INDEX_6,
        SAME_INDEX_7,
        SAME_INDEX_8,
    };

    for (int phase = 0; phase < NB_PHASE; phase++)
    {
        int ftype;
        regr[phase].beta = BETA_INIT;
        for (ftype = 0; ftype < FEAT_TYPE_NUM; ftype++)
        {
            regr[phase].nbAppears[ftype] = (uint32_t *)malloc(sizeof(uint32_t) * FTYPE_INDEX_MAX[ftype]);
            regr[phase].delta[ftype] = (double *)malloc(sizeof(double) * FTYPE_INDEX_MAX[ftype]);
        }
    }

    int digitStart = 4;
    // 単純反転インデックスを計算
    for (int digit = digitStart; digit <= 8; digit++)
    {
        for (int i = 0; i < POW3_LIST[digit]; i++)
        {
            int idx = i;
            uint16_t same = 0;
            for (int j = 0; j < digit; j++)
            {
                // 3進1桁目を取り出してj桁左シフト
                same += idx % 3 * POW3_LIST[digit - j - 1];
                // idx右シフト
                idx /= 3;
            }
            SIMPLE_SAMES[digit - digitStart][i] = same;
        }
    }

    // CORNRの対称インデックスを計算
    static uint16_t corn_revs[9] = {POW3_0, POW3_3, POW3_6, POW3_1, POW3_4, POW3_7, POW3_2, POW3_5, POW3_8};
    for (int i = 0; i < POW3_9; i++)
    {
        int idx = i;
        uint16_t same = 0;
        for (int j = 0; j < 9; j++)
        {
            same += idx % 3 * corn_revs[j];
            idx /= 3;
        }
        SAME_INDEX_CORNR[i] = same;
    }

    // BOX10の対称インデックスを計算
    /*
    static uint16_t bmran_revs[8] = {POW3_0, POW3_4, POW3_6, POW3_7, POW3_1, POW3_5, POW3_2, POW3_3};
    for (i = 0; i < POW3_10; i++)
    {
        same = 0;
        idx = i;
        for (j = 0; j < 10; j++)
        {
            same += idx % 3 * bmran_revs[j];
            idx /= 3;
        }
        SAME_INDEX_BOX10[i] = same;
    }*/

    // EDGEの対称インデックスを計算
    for (int i = 0; i < POW3_10; i++)
    {
        // 0120120120 -> 0210210210
        uint16_t same = 0;
        int idx = i;
        for (int j = 0; j < 10; j++)
        {
            same += idx % 3 * POW3_LIST[9 - j];
            idx /= 3;
        }
        SAME_INDEX_EDGE[i] = same;
    }
}

double RegrTrain(Regressor regr[NB_PHASE], vector<FeatureRecord> &featRecords, FeatureRecord *testRecords, size_t nbTests)
{
    double loss, totalLoss;
    int i, startIdx, endIdx;
    int phase, batchIdx, testCnt, totalCnt, nbEmpty;
    int testSize[NB_PHASE] = {0};
    int testTmpSize[NB_PHASE] = {0};
    vector<FeatureRecord *> inputs[60];
    vector<FeatureRecord *> phaseInputs;
    FeatureRecord **tests[NB_PHASE];
    FeatureRecord *batchInput[BATCH_SIZE];

    for (i = 0; i < nbTests; i++)
    {
        testSize[PHASE(testRecords[i].nbEmpty)]++;
    }

    for (phase = 0; phase < NB_PHASE; phase++)
    {
        tests[phase] = (FeatureRecord **)malloc(sizeof(FeatureRecord *) * testSize[phase]);
    }

    // レコードをフェーズごとに振り分け
    for (i = 0; i < featRecords.size(); i++)
    {
        inputs[featRecords[i].nbEmpty].push_back(&featRecords[i]);
    }
    for (i = 0; i < nbTests; i++)
    {
        phase = PHASE(testRecords[i].nbEmpty);
        tests[phase][testTmpSize[phase]] = &testRecords[i];
        testTmpSize[phase]++;
    }

    totalLoss = 0;
    totalCnt = 0;
    // すべてのフェーズに対して学習
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        // フェーズ内の対戦記録を取得
        phaseInputs.clear();
        // フェーズ前後の局面も学習（スムージング）
        startIdx = phase * NB_PUT_1PHASE - (NB_PUT_1PHASE / 2);
        endIdx = (phase + 1) * NB_PUT_1PHASE + (NB_PUT_1PHASE / 2);
        if (startIdx < 0)
            startIdx = 0;
        if (endIdx > 60)
            endIdx = 60;
        for (nbEmpty = startIdx; nbEmpty < endIdx; nbEmpty++)
        {
            for (i = 0; i < inputs[nbEmpty].size(); i++)
            {
                phaseInputs.push_back(inputs[nbEmpty][i]);
            }
            //phaseInputs.reserve(phaseInputs.size() + inputs[nbEmpty].size());
            //copy(inputs[nbEmpty].begin(), inputs[nbEmpty].end(), back_inserter(phaseInputs));
        }

        // ミニバッチ学習
        for (batchIdx = 0; batchIdx < phaseInputs.size(); batchIdx += BATCH_SIZE)
        {
            printf("Regressor train phase:%d batch:%d      \r", phase, batchIdx / BATCH_SIZE);
            // バッチサイズ分ランダムサンプリング
            sampling(phaseInputs, batchInput, BATCH_SIZE);
            TreinRegrBatch(&regr[phase], batchInput, BATCH_SIZE);
        }

        loss = 0;
        testCnt = 0;
        for (i = 0; i < testSize[phase]; i++)
        {

            loss += fabs(tests[phase][i]->stoneDiff - RegrPred(&regr[phase], tests[phase][i]->featStats[0], 0));
            testCnt++;
            totalCnt++;
        }
        totalLoss += loss;
        printf("Regressor phase%d  MAE Loss: %.3f          \n", phase, loss / (double)testCnt);
    }

    for (phase = 0; phase < NB_PHASE; phase++)
    {
        free(tests[phase]);
    }
    for (i = 0; i < 60; i++)
    {
        inputs[i].clear();
    }
    phaseInputs.clear();
    return (double)totalLoss / totalCnt;
}
#endif