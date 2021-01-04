#include "regression.h"
#include "../ai/eval.h"
#include <stdlib.h>
#include <stdio.h>

#define BATCH_SIZE 64

static const float beta = 0.01;

void InitRegressor(Regressor regr[NB_PHASE])
{
    int phase, i;
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        for (i = 0; i < FEAT_NB_COMBINATION; i++)
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
        score += regr->weights[features[featIdx]];
        shift += FeatMaxIndex[featIdx];
    }
    // EDGEは1,2 3,4 5,6 7,8を統合する
    // (NNでは位置を学習・統合できるが，線形回帰は隣り合ったパターンを認識できないため)
    for (; featIdx < FEAT_EDGEX_8; featIdx += 2)
    {
        score += regr->weights[features[featIdx] * POW3_5 + features[featIdx + 1]];
        shift += POW3_5;
        shift += POW3_5;
    }
    for (; featIdx < FEAT_NUM; featIdx++)
    {
        score += regr->weights[features[featIdx]];
        shift += FeatMaxIndex[featIdx];
    }
    return score;
}

void IntegrateRegrWeight();

void UpdateRegrWeights(Regressor *regr)
{
    int j;
    float alpha;
    for (j = 0; j < REGR_NB_FEAT_COMB; j++)
    {
        alpha = min(beta / 50.0f, beta / (float)regr->nbAppear[j]);
        regr->weights[j] += alpha * regr->delta[j];
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
    for (; featIdx < FEAT_EDGEX_8; featIdx += 2)
    {
        feat = shift + features[featIdx] * POW3_5 + features[featIdx + 1];
        regr->nbAppear[feat]++;
        regr->delta[feat] += error;

        shift += POW3_5;
        shift += POW3_5;
    }
    for (; featIdx < FEAT_NUM; featIdx++)
    {
        feat = shift + features[featIdx];
        regr->nbAppear[feat]++;
        regr->delta[feat] += error;

        shift += FeatMaxIndex[featIdx];
    }
}

float TreinRegrBatch(Regressor *regr, FeatureRecord *inputs[BATCH_SIZE], int inputSize)
{
    int i;
    float teacher, output, loss = 0;
    for (i = 0; i < inputSize; i++)
    {
        teacher = inputs[i]->stoneDiff;
        output = PredRegressor(regr, inputs[i]->featStats);
        CalcWeightDelta(regr, inputs[i]->featStats, teacher - output);
        loss += fabs(teacher - output);
    }
    UpdateRegrWeights(regr);
    return loss / inputSize;
}

void sampling(FeatureRecord **records, FeatureRecord **sampledList, int nbRecords, int nbSample)
{
    int i;
    uint32 randIdx;
    for (i = 0; i < nbSample; i++)
    {
        // 0~100万の乱数を作ってレコード数で丸め
        randIdx = ((rand() % 1000) * 1000 + (rand() % 1000)) % nbRecords;
        sampledList[i] = records[randIdx];
    }
}

void TrainRegressor(Regressor regr[NB_PHASE], FeatureRecord *featRecords, size_t nbRecords)
{
    double loss;
    int i, phase, batchIdx, learndCnt;
    int inputSize[NB_PHASE] = {0, 0, 0, 0};
    int tmpSize[NB_PHASE] = {0, 0, 0, 0};
    FeatureRecord **inputs[NB_PHASE];
    FeatureRecord *batchInput[BATCH_SIZE];
    for (i = 0; i < nbRecords; i++)
    {
        inputSize[PHASE(featRecords[i].nbEmpty)]++;
    }

    for (phase = 0; phase < NB_PHASE; phase++)
    {
        inputs[phase] = (FeatureRecord **)malloc(sizeof(FeatureRecord *) * inputSize[phase]);
    }

    // レコードをフェーズごとに振り分け
    for (i = 0; i < nbRecords; i++)
    {
        phase = PHASE(featRecords[i].nbEmpty);
        inputs[phase][tmpSize[phase]] = &featRecords[i];
        tmpSize[phase]++;
    }

    // すべてのフェーズに対して学習
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        loss = 0;
        learndCnt = 0;
        // ミニバッチ学習
        for (batchIdx = 0; batchIdx < inputSize[phase]; batchIdx += BATCH_SIZE)
        {
            printf("Regressor train phase:%d batch:%d      \r", phase, batchIdx / BATCH_SIZE);
            // バッチサイズ分ランダムサンプリング
            sampling(inputs[phase], batchInput, inputSize[phase], BATCH_SIZE);
            loss += TreinRegrBatch(&regr[phase], batchInput, BATCH_SIZE);
            learndCnt += BATCH_SIZE;
        }
        printf("Regressor phase%d  MAE Loss: %.3f          \n", phase, loss / (float)learndCnt);
    }

    for (phase = 0; phase < NB_PHASE; phase++)
    {
        free(inputs[phase]);
    }
}

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
        if (readed < 1)
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