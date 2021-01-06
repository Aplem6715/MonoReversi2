
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

void IntegrateRegrWeight();

void UpdateRegrWeights(Regressor *regr)
{
    int j;
    float alpha;
    for (j = 0; j < REGR_NB_FEAT_COMB; j++)
    {
        alpha = fminf(regr->beta / 50.0f, regr->beta / (float)regr->nbAppear[j]);
        regr->weights[j] += alpha * regr->delta[j];
        regr->nbAppear[j] = 0;
        regr->delta[j] = 0;

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
    }
}

void ResetRegrState(Regressor *regr)
{
    int featIdx;
    for (featIdx = 0; featIdx < REGR_NB_FEAT_COMB; featIdx++)
    {
        regr->nbAppear[featIdx] = 0;
        regr->delta[featIdx] = 0;
    }
}

void CalcWeightDelta(Regressor *regr, const uint16 features[], float error)
{
    int featIdx, feat;
    uint32 shift = 0;
    for (featIdx = 0; featIdx < FEAT_EDGEX_1; featIdx++)
    {
        feat = shift + features[featIdx];
        regr->nbAppear[feat]++;
        regr->delta[feat] += error;

        shift += FeatMaxIndex[featIdx];
    }
    // EDGEは1,2 3,4 5,6 7,8を統合する
    // (NNでは位置を学習・統合できるが，線形回帰は隣り合ったパターンを認識できないため)
    for (; featIdx <= FEAT_EDGEX_8; featIdx += 2)
    {
        feat = shift + features[featIdx] * POW3_5 + features[featIdx + 1];
        regr->nbAppear[feat]++;
        regr->delta[feat] += error;

        shift += FeatMaxIndex[featIdx] * FeatMaxIndex[featIdx + 1];
    }
    for (; featIdx < FEAT_NUM; featIdx++)
    {
        feat = shift + features[featIdx];
        regr->nbAppear[feat]++;
        regr->delta[feat] += error;

        shift += FeatMaxIndex[featIdx];
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
            Board::Draw(inputs[i]->own, inputs[i]->opp, 0);
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