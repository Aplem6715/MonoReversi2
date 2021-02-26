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

#define BATCH_SIZE 512
static const double BETA_INIT = 0.001f;

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
};

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
    int featType, feat, rot;
    double alpha;
    uint16_t idx, featIdx;
    int32_t sameIdx;

    uint32_t appearSum;
    double delSum;

    // タイプごとの対象型インデックスへの参照を配列として持っておく
    uint16_t *FeatTypeSames[NB_FEATURE_TYPES - 1] = {
        SAME_INDEX_8,    //LINE2
        SAME_INDEX_8,    //LINE3
        SAME_INDEX_8,    //LINE4
        SAME_INDEX_4,    //DIAG4
        SAME_INDEX_5,    //DIAG5
        SAME_INDEX_6,    //DIAG6
        SAME_INDEX_7,    //DIAG7
        SAME_INDEX_8,    // DIAG8
        SAME_INDEX_EDGE, //
        SAME_INDEX_CORNR //
    };

    feat = 0;
    // 各パターンタイプについてループ
    for (featType = 0; featType < NB_FEATURE_TYPES; featType++)
    {
        // 0~そのパターンタイプが取りうるインデックスの最大値までループ
        for (idx = 0; idx < FeatTypeMaxIndex[featType]; idx++)
        {
            appearSum = 0;
            delSum = 0;
            if (featType == FEAT_TYPE_BOX10)
            {
                sameIdx = -1;
            }
            else
            {
                sameIdx = FeatTypeSames[featType][idx];
            }

            appearSum += regr->nbAppears[feat][idx];
            delSum += regr->del[feat][idx];

            alpha = fmin(regr->beta / 50.0f, regr->beta / (double)appearSum);
            // ウェイト調整
            regr->weight[0][feat][idx] += alpha * delSum;

            /*
            // 同タイプ，同インデックス，対象型の出現回数と誤差の合計を求める
            for (rot = 0; rot < FeatTypeNbRots[featType]; rot++)
            {
                featIdx = feat + rot;

                appearSum += regr->nbAppears[featIdx][idx];
                delSum += regr->del[featIdx][idx];
                // 対象型があればその分も加算
                if (sameIdx >= 0)
                {
                    appearSum += regr->nbAppears[featIdx][sameIdx];
                    delSum += regr->del[featIdx][sameIdx];
                }
                assert(appearSum < 4294967296);
            }

            // 同タイプ，同インデックス，対象型のウェイトを更新
            for (rot = 0; rot < FeatTypeNbRots[featType]; rot++)
            {
                alpha = fmin(regr->beta / 50.0f, regr->beta / (double)appearSum);
                // ウェイト調整
                regr->weight[0][feat + rot][idx] += alpha * delSum;
                // 対象型があれば調整
                if (sameIdx >= 0)
                {
                    regr->weight[0][feat + rot][sameIdx] += alpha * delSum;
                }
            }
        }
        */
            feat += FeatTypeNbRots[featType];
        }
    }

    void ResetRegrState(Regressor * regr)
    {
        uint16_t featIdx, i;
        for (featIdx = 0; featIdx < FEAT_NUM; featIdx++)
        {
            for (i = 0; i < FeatMaxIndex[featIdx]; i++)
            {
                regr->nbAppears[featIdx][i] = 0;
                regr->del[featIdx][i] = 0;
            }
        }
    }

    void CalcWeightDelta(Regressor * regr, const uint16_t features[FEAT_NUM], double error)
    {
        int featType;

        for (featType = 0; featType < FEAT_NUM; featType++)
        {
            regr->nbAppears[featType][features[featType]]++;
            regr->del[featType][features[featType]] += error;
            assert(regr->nbAppears[featType][features[featType]] < 4294967296);
        }
    }

    void TreinRegrBatch(Regressor * regr, FeatureRecord * inputs[BATCH_SIZE], int inputSize)
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
        //RegrApplyWeightToOpp(regr);
    }

    void RegrTrainInit(Regressor regr[NB_PHASE])
    {
        int phase, i;

        static uint16_t *SIMPLE_SAMES[] = {
            SAME_INDEX_4,
            SAME_INDEX_5,
            SAME_INDEX_6,
            SAME_INDEX_7,
            SAME_INDEX_8,
        };

        for (phase = 0; phase < NB_PHASE; phase++)
        {
            int featIdx;
            regr[phase].beta = BETA_INIT;
            for (featIdx = 0; featIdx < FEAT_NUM; featIdx++)
            {
                regr[phase].nbAppears[featIdx] = (uint32_t *)malloc(sizeof(uint32_t) * FeatMaxIndex[featIdx]);
                regr[phase].del[featIdx] = (double *)malloc(sizeof(double) * FeatMaxIndex[featIdx]);
            }
        }

        uint16_t same, idx;
        int j, digit, digitStart = 4;
        // 単純反転インデックスを計算
        for (digit = digitStart; digit <= 8; digit++)
        {
            for (i = 0; i < POW3_LIST[digit]; i++)
            {
                idx = i;
                same = 0;
                for (j = 0; j < digit; j++)
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
        for (i = 0; i < POW3_9; i++)
        {
            same = 0;
            idx = i;
            for (j = 0; j < 9; j++)
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
        for (i = 0; i < POW3_10; i++)
        {
            // 0120120120 -> 0210210210
            same = 0;
            idx = i;
            for (j = 0; j < 10; j++)
            {
                same += idx % 3 * POW3_LIST[9 - j];
                idx /= 3;
            }
            SAME_INDEX_EDGE[i] = same;
        }
    }

    double RegrTrain(Regressor regr[NB_PHASE], vector<FeatureRecord> & featRecords, FeatureRecord * testRecords, size_t nbTests)
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