
#define _CRT_SECURE_NO_WARNINGS
#include "regression.h"
#include "../ai/eval.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "../board.h"

#define BATCH_SIZE 128
static const float BETA_INIT = 0.001f;

static const uint32 FeatMaxIndex[] = {
    POW3_8, POW3_8, POW3_8, POW3_8,     // LINE2  26244
    POW3_8, POW3_8, POW3_8, POW3_8,     // LINE3  26244
    POW3_8, POW3_8, POW3_8, POW3_8,     // LINE4  26244
    POW3_4, POW3_4, POW3_4, POW3_4,     // DIAG4    324
    POW3_5, POW3_5, POW3_5, POW3_5,     // DIAG5    972
    POW3_6, POW3_6, POW3_6, POW3_6,     // DIAG6   2916
    POW3_7, POW3_7, POW3_7, POW3_7,     // DIAG7   8748
    POW3_8, POW3_8,                     // DIAG8  13122
    POW3_10, POW3_10, POW3_10, POW3_10, // EDGEX 3^10x4
    POW3_8, POW3_8, POW3_8, POW3_8,     // CORNR  26244
    POW3_8, POW3_8, POW3_8, POW3_8      // BMRAN  26244
};

static const uint32 FeatTypeMaxIndex[] = {
    POW3_8,  // LINE2  26244
    POW3_8,  // LINE3  26244
    POW3_8,  // LINE4  26244
    POW3_4,  // DIAG4    324
    POW3_5,  // DIAG5    972
    POW3_6,  // DIAG6   2916
    POW3_7,  // DIAG7   8748
    POW3_8,  // DIAG8  13122
    POW3_10, // EDGEX 3^10x4
    POW3_8,  // CORNR  26244
    POW3_8,  // BMRAN  26244
};

static const uint16 POW3_LIST[] = {POW3_0, POW3_1, POW3_2, POW3_4, POW3_5, POW3_6, POW3_7, POW3_8, POW3_9, POW3_10};
static uint16 SAME_INDEX_8[POW3_8];
static uint16 SAME_INDEX_7[POW3_7];
static uint16 SAME_INDEX_6[POW3_6];
static uint16 SAME_INDEX_5[POW3_5];
static uint16 SAME_INDEX_4[POW3_4];
static uint16 SAME_INDEX_3[POW3_3];
static uint16 SAME_INDEX_CORNR[POW3_9];
static uint16 SAME_INDEX_BMRAN[POW3_9];
static uint16 SAME_INDEX_EDGE[POW3_10];

static uint16 *SAMES[] = {
    SAME_INDEX_3,
    SAME_INDEX_4,
    SAME_INDEX_5,
    SAME_INDEX_6,
    SAME_INDEX_7,
    SAME_INDEX_8,
};

void InitRegrBeta(Regressor regr[NB_PHASE])
{
    int phase, i;
    for (phase = 0; phase < NB_PHASE; phase++)
    {
#ifdef LEARN_MODE
        regr[phase].beta = BETA_INIT;
#endif
    }
}

void InitRegressor(Regressor regr[NB_PHASE])
{
    int phase, i;
    for (phase = 0; phase < NB_PHASE; phase++)
    {
#ifdef LEARN_MODE
        regr[phase].beta = BETA_INIT;
#endif
        for (i = 0; i < REGR_NB_FEAT_COMB; i++)
        {
            regr[phase].weights[i] = 0;
        }
    }
#ifdef LEARN_MODE
    uint16 same, idx;
    int j, digit, digitStart = 3;
    // 単純反転インデックスを計算
    for (digit = digitStart; digit < 8; digit++)
    {
        for (i = 0; i < POW3_LIST[digit]; i++)
        {
            idx = i;
            same = 0;
            for (j = 0; j < 8; j++)
            {
                // 3進1桁目を取り出してj桁左シフト
                same += idx % 3 * POW3_LIST[j];
                // idx右シフト
                idx /= 3;
            }
            SAMES[digit - digitStart][i] = same;
        }
    }

    // CORNRの対称インデックスを計算
    static uint16 corn_revs[8] = {POW3_0, POW3_3, POW3_6, POW3_1, POW3_4, POW3_7, POW3_2, POW3_5};
    for (i = 0; i < POW3_8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            same += idx % 3 * corn_revs[j];
            idx /= 3;
        }
        SAME_INDEX_CORNR[i] = same;
    }

    // BMRANの対称インデックスを計算
    static uint16 bmran_revs[8] = {POW3_0, POW3_4, POW3_6, POW3_7, POW3_1, POW3_5, POW3_2, POW3_3};
    for (i = 0; i < POW3_8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            same += idx % 3 * bmran_revs[j];
            idx /= 3;
        }
        SAME_INDEX_BMRAN[i] = same;
    }

    // 連結EDGEの対称インデックスを計算
    static uint16 edge_revs[10] = {POW3_5, POW3_6, POW3_7, POW3_8, POW3_9, POW3_0, POW3_1, POW3_2, POW3_3, POW3_4};
    for (i = 0; i < POW3_10; i++)
    {
        for (j = 0; j < 10; j++)
        {
            same += idx % 3 * edge_revs[j];
            idx /= 3;
        }
        SAME_INDEX_EDGE[i] = same;
    }
#endif
}

float PredRegressor(Regressor *regr, const uint16 features[])
{
    int featIdx;
    float score = 0;
    uint32 shift = 0;
    for (featIdx = 0; featIdx < FEAT_EDGEX_1; featIdx++)
    {
        score += regr->weights[shift + features[featIdx]];
        shift += FeatMaxIndex[featIdx];
        assert(shift < REGR_NB_FEAT_COMB);
    }
    // EDGEは1,2 3,4 5,6 7,8を統合する
    // (NNでは位置を学習・統合できるが，線形回帰は隣り合ったパターンを認識できないため)
    for (; featIdx < FEAT_EDGEX_8; featIdx += 2)
    {
        score += regr->weights[shift + features[featIdx] * POW3_5 + features[featIdx + 1]];
        shift += FeatMaxIndex[featIdx] * FeatMaxIndex[featIdx + 1];
        assert(shift < REGR_NB_FEAT_COMB);
    }

    for (; featIdx < FEAT_NUM; featIdx++)
    {
        assert(shift < REGR_NB_FEAT_COMB);
        score += regr->weights[shift + features[featIdx]];
        shift += FeatMaxIndex[featIdx];
    }
    return score;
}

#ifdef LEARN_MODE

void DecreaseRegrBeta(Regressor regr[NB_PHASE])
{
    int phase;
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        regr[phase].beta /= 2.0f;
    }
}

void IntegrateRegrWeight()
{
}

void UpdateRegrWeights(Regressor *regr)
{
    int j, featType;
    float alpha;
    for (featType = 0; featType < REGR_FEAT_NUM; featType++)
    {
    }
    for (j = 0; j < REGR_NB_FEAT_COMB; j++)
    {
        alpha = fminf(regr->beta / 50.0f, regr->beta / (float)regr->nbAppear[j]);
        regr->weights[j] += alpha * regr->delta[j];
        regr->nbAppear[j] = 0;
        regr->delta[j] = 0;
        /*
        if (regr->weights[j] > 64)
        {
            //printf("max\n");
            regr->weights[j] = 64;
        }
        if (regr->weights[j] < -64)
        {
            //printf("min\n");
            regr->weights[j] = -64;
        }
        */
    }
}

void ResetRegrState(Regressor *regr)
{
    int featType, i;
    for (featType = 0; featType < REGR_FEAT_TYPES; featType++)
    {
        for (i = 0; i < FeatTypeMaxIndex[featType]; i++)
        {
            regr->nbAppears[featType][i] = 0;
            regr->del[featType][i] = 0;
        }
    }
}

void CalcWeightDelta(Regressor *regr, const uint16 features[FEAT_NUM], float error)
{
    int featIdx, feat, featType, edgeShift, idx;
    uint32 shift = 0;

    // TODO: featTypeはLINE, DIAG3,4,5,6,7,8, EDGE, CORNR, BMRANの0～11,
    //       featuresはLINE_1, LINE_2, ..., となり約4倍になる。
    for (featType = 0; featType < FEAT_EDGEX_1; featType++)
    {
        regr->nbAppears[featType][features[featType]]++;
        regr->del[featType][features[featType]] += error;
    }
    for (edgeShift = 0; featType <= FEAT_EDGEX_4; featType++, edgeShift += 2)
    {
        idx = features[idx] * POW3_5 + features[idx + 1];
        regr->nbAppears[featType][idx]++;
        regr->del[featType][idx] += error;
    }
    for (; featType < REGR_FEAT_NUM; featType++)
    {
        regr->nbAppears[featType][features[featType + 4]]++;  // EDGE連結で減った分+4
        regr->del[featType][features[featType + 4]] += error; // EDGE連結で減った分+4
    }
}

void TreinRegrBatch(Regressor *regr, FeatureRecord *inputs[BATCH_SIZE], int inputSize)
{
    int i;
    static bool debug = false;
    float teacher, output, loss = 0;

    ResetRegrState(regr);
    for (i = 0; i < inputSize; i++)
    {
        teacher = inputs[i]->stoneDiff;
        output = PredRegressor(regr, inputs[i]->featStats);
        CalcWeightDelta(regr, inputs[i]->featStats, teacher - output);
        if (debug)
        {
            //Board::Draw(inputs[i]->own, inputs[i]->opp, 0);
            printf("Color: %d,  Score: %f, Pred: %f\n", inputs[i]->color, teacher, output);
        }
    }
    UpdateRegrWeights(regr);
}

float TrainRegressor(Regressor regr[NB_PHASE], FeatureRecord *featRecords, FeatureRecord *testRecords, size_t nbRecords, size_t nbTests)
{
    double loss, totalLoss;
    int i, phase, batchIdx, testCnt, totalCnt;
    int testSize[NB_PHASE] = {0};
    int testTmpSize[NB_PHASE] = {0};
    vector<FeatureRecord *> inputs[NB_PHASE];
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
    for (i = 0; i < nbRecords; i++)
    {
        phase = PHASE(featRecords[i].nbEmpty);
        inputs[phase].push_back(&featRecords[i]);
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
        // ミニバッチ学習
        for (batchIdx = 0; batchIdx < inputs[phase].size(); batchIdx += BATCH_SIZE)
        {
            printf("Regressor train phase:%d batch:%d      \r", phase, batchIdx / BATCH_SIZE);
            // バッチサイズ分ランダムサンプリング
            sampling(inputs[phase], batchInput, BATCH_SIZE);
            TreinRegrBatch(&regr[phase], batchInput, BATCH_SIZE);
        }

        loss = 0;
        testCnt = 0;
        for (i = 0; i < testSize[phase]; i++)
        {
            loss += fabsf(tests[phase][i]->stoneDiff - PredRegressor(&regr[phase], tests[phase][i]->featStats));
            testCnt++;
            totalCnt++;
        }
        totalLoss += loss;
        printf("Regressor phase%d  MAE Loss: %.3f          \n", phase, loss / (float)testCnt);
    }

    for (phase = 0; phase < NB_PHASE; phase++)
    {
        free(tests[phase]);
    }
    return (float)totalLoss / totalCnt;
}
#endif

void SaveRegressor(Regressor regr[NB_PHASE], const char *file)
{
    int phase;
    size_t writed;
    char fileName[100];
    FILE *fp;
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        sprintf(fileName, "%sphase%d", file, phase);
        fp = fopen(fileName, "wb");
        if (fp == NULL)
        {
            fputs("書き込み用モデルファイルオープンに失敗しました。\n", stderr);
            exit(EXIT_FAILURE);
        }

        writed = 0;
        writed += fwrite(&regr[phase].weights, sizeof(float), REGR_NB_FEAT_COMB, fp);
        if (writed < 1)
        {
            fputs("モデルファイルへの書き込みに失敗しました。\n", stderr);
            exit(EXIT_FAILURE);
        }

        if (fclose(fp) == EOF)
        {
            fputs("モデルファイルクローズに失敗しました。\n", stderr);
            exit(EXIT_FAILURE);
        }
    }
}
void LoadRegressor(Regressor regr[NB_PHASE], const char *file)
{
    int phase;
    size_t readed;
    char fileName[100];
    FILE *fp;
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        sprintf(fileName, "%sphase%d", file, phase);
        fp = fopen(fileName, "rb");
        if (fp == NULL)
        {
            fputs("読み込み用モデルファイルオープンに失敗しました。\n", stderr);
            return;
        }

        readed = 0;
        readed += fread(&regr[phase].weights, sizeof(float), REGR_NB_FEAT_COMB, fp);
        if (readed < REGR_NB_FEAT_COMB)
        {
            fputs("モデルファイルの読み込みに失敗しました。\n", stderr);
            return;
        }

        if (fclose(fp) == EOF)
        {
            fputs("モデルファイルクローズに失敗しました。\n", stderr);
            return;
        }
    }
}