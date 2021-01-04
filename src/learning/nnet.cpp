#include "nnet.h"
#include "../ai/eval.h"

#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define BATCH_SIZE 32
#define WEIGHT_SEED 42
#define HE_COEFF 2.0f
static const float lr = 0.01f;

void InitWeight(NNet net[NB_PHASE])
{
    int i, j, phase;
    srand(WEIGHT_SEED);
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        for (j = 0; j < VALUE_HIDDEN_UNITS1; j++)
        {
            for (i = 0; i < FEAT_NB_COMBINATION; i++)
            {
                net[phase].c1[i][j] = rand() / (1.0f + RAND_MAX) * sqrtf(HE_COEFF / FEAT_NUM);
            }
            // バイアス
            net[phase].c1[i][j] = 0;
        }
        for (j = 0; j < VALUE_HIDDEN_UNITS2; j++)
        {
            for (i = 0; i < VALUE_HIDDEN_UNITS1; i++)
            {
                net[phase].c2[i][j] = rand() / (1.0f + RAND_MAX) * sqrtf(HE_COEFF / VALUE_HIDDEN_UNITS1);
            }
            // バイアス
            net[phase].c2[i][j] = 0;
        }
        for (i = 0; i < VALUE_HIDDEN_UNITS2; i++)
        {
            net[phase].c3[i][0] = rand() / (1.0f + RAND_MAX) * sqrtf(HE_COEFF / VALUE_HIDDEN_UNITS2);
        }
        // バイアス
        net[phase].c3[i][0] = 0;
    }
}

float act(float x)
{
    /*
    if (x < 0)
    {
        return 0;
    }
    return x;
    */
    // tanhExp関数
    return x * (float)tanh(exp(x));
}

float d_act(float x)
{
    /*
    if (x < 0)
    {
        return 0;
    }
    else
    {
        return 1.0f;
    }
    */
    // tanhExp微分関数
    float e_x = expf(x);
    float tanh_e_x = tanhf(e_x);
    return tanh_e_x - x * e_x * (tanh_e_x * tanh_e_x - 1);
}

float mse(float y, float t)
{
    float diff = t - y;
    return diff * diff / 2;
}

float d_mse(float y, float t)
{
    return (y - t);
}

float forward(NNet *net, const uint16 features[FEAT_NUM], uint8 isTrain)
{
    static float holder1[VALUE_HIDDEN_UNITS1];
    static float holder2[VALUE_HIDDEN_UNITS2];

    uint32 shift = 0;
    int unitIdx;
    int prevUnit;
    int featIdx;
    float bias, sum = 0;
    float output;

    // 入力層 -> 中間層１
    for (unitIdx = 0; unitIdx < VALUE_HIDDEN_UNITS1; unitIdx++)
    {
        sum = 0;
        shift = 0;
        bias = net->c1[FEAT_NB_COMBINATION][unitIdx];
        for (featIdx = 0; featIdx < FEAT_NUM; featIdx++)
        {
            sum += net->c1[shift + features[featIdx]][unitIdx] * 1;
            sum += bias;
#ifdef LEARN_MODE
            if (isTrain)
            {
                net->state1[shift + features[featIdx]][unitIdx].sum = sum;
            }
#endif // LEARN_MODE
            shift += FeatMaxIndex[featIdx];
        }
        holder1[unitIdx] = act(sum);
    }

    // 中間層1 -> 中間層2
    for (unitIdx = 0; unitIdx < VALUE_HIDDEN_UNITS2; unitIdx++)
    {
        sum = 0;
        bias = net->c2[VALUE_HIDDEN_UNITS1][unitIdx];
        for (prevUnit = 0; prevUnit < VALUE_HIDDEN_UNITS1; prevUnit++)
        {
            sum += net->c2[prevUnit][unitIdx] * holder1[prevUnit];
            sum += bias;
#ifdef LEARN_MODE
            if (isTrain)
            {
                net->state2[prevUnit][unitIdx].sum = sum;
            }
#endif // LEARN_MODE
        }
        holder2[unitIdx] = act(sum);
    }

    // 中間層2 -> 出力層
    for (unitIdx = 0; unitIdx < 1; unitIdx++)
    {
        sum = 0;
        bias = net->c3[VALUE_HIDDEN_UNITS2][unitIdx];
        for (prevUnit = 0; prevUnit < VALUE_HIDDEN_UNITS2; prevUnit++)
        {
            sum += net->c3[prevUnit][unitIdx] * holder2[prevUnit];
            sum += bias;
        }
#ifdef LEARN_MODE
        if (isTrain)
        {
            net->state3[prevUnit][unitIdx].sum = sum;
        }
#endif // LEARN_MODE
        output = sum;
    }

    return output;
}

void backward(NNet *net, const uint16 features[FEAT_NUM], float y, float t)
{
    UnitState *targetStat;
    int j;
    int i;
    int nextUnit;
    float delta_w_sum;
    // 出力　-> 中間2
    net->state3[VALUE_HIDDEN_UNITS2][0].delta = 0;
    for (i = 0; i < VALUE_HIDDEN_UNITS2; i++)
    {
        targetStat = &net->state3[i][0];
        targetStat->delta = -(y - t) * d_act(targetStat->sum);
        targetStat->dw_sum += targetStat->delta * act(targetStat->sum);
        net->state3[VALUE_HIDDEN_UNITS2][0].delta += targetStat->delta;
    }
    // バイアス
    net->state3[VALUE_HIDDEN_UNITS2][0].dw_sum += net->state3[VALUE_HIDDEN_UNITS2][0].delta;

    // 中間2 -> 中間1
    for (j = 0; j < VALUE_HIDDEN_UNITS2; j++)
    {
        net->state2[VALUE_HIDDEN_UNITS1][j].delta = 0;
        delta_w_sum = net->c3[j][0] * net->state3[j][0].delta;
        for (i = 0; i < VALUE_HIDDEN_UNITS1; i++)
        {
            // delta_h2_j = f'(i_j) * Sigma[delta_out * w_j]
            // dE/dw_j = delta
            targetStat = &net->state2[i][j];
            targetStat->delta = d_act(targetStat->sum) * delta_w_sum;
            targetStat->dw_sum += targetStat->delta * act(targetStat->sum);
            net->state2[VALUE_HIDDEN_UNITS1][j].delta += targetStat->delta;
        }
        // バイアス
        net->state2[VALUE_HIDDEN_UNITS1][j].dw_sum += net->state2[VALUE_HIDDEN_UNITS1][j].delta;
    }

    // 中間1 -> 入力
    uint32 shift;
    uint16 featIdx;
    for (j = 0; j < VALUE_HIDDEN_UNITS1; j++)
    {
        delta_w_sum = 0;
        for (nextUnit = 0; nextUnit < VALUE_HIDDEN_UNITS2; nextUnit++)
        {
            // [delta_out * w_j]
            delta_w_sum += net->c2[j][nextUnit] * net->state2[j][nextUnit].delta;
        }

        shift = 0;
        net->state1[FEAT_NB_COMBINATION][j].delta = 0;
        for (featIdx = 0; featIdx < FEAT_NUM; featIdx++)
        {
            i = shift + features[featIdx];
            shift += FeatMaxIndex[featIdx];

            targetStat = &net->state1[i][j];
            targetStat->delta = d_act(targetStat->sum) * delta_w_sum;
            targetStat->dw_sum += targetStat->delta * act(targetStat->sum);
            net->state1[FEAT_NB_COMBINATION][j].delta += targetStat->delta;
        }
        // バイアス
        net->state1[FEAT_NB_COMBINATION][j].dw_sum += net->state1[FEAT_NB_COMBINATION][j].delta;
    }
}

void update_weights(NNet *net, int batchSize)
{
    int i, j;
    for (j = 0; j < VALUE_HIDDEN_UNITS1; j++)
    {
        for (i = 0; i < FEAT_NB_COMBINATION; i++)
        {
            net->c1[i][j] += lr * (net->state1[i][j].dw_sum / (float)batchSize);
            net->state1[i][j].dw_sum = 0;
        }
        // バイアス
        net->c1[FEAT_NB_COMBINATION][j] += lr * (net->state1[FEAT_NB_COMBINATION][j].dw_sum / (float)batchSize);
        net->state1[FEAT_NB_COMBINATION][j].dw_sum = 0;
    }
    for (j = 0; j < VALUE_HIDDEN_UNITS2; j++)
    {
        for (i = 0; i < VALUE_HIDDEN_UNITS1; i++)
        {
            net->c2[i][j] += lr * (net->state2[i][j].dw_sum / (float)batchSize);
            net->state2[i][j].dw_sum = 0;
        }
        // バイアス
        net->c2[VALUE_HIDDEN_UNITS1][j] += lr * (net->state2[VALUE_HIDDEN_UNITS1][j].dw_sum / (float)batchSize);
        net->state2[VALUE_HIDDEN_UNITS1][j].dw_sum = 0;
    }
    for (j = 0; j < 1; j++)
    {
        for (i = 0; i < VALUE_HIDDEN_UNITS2; i++)
        {
            net->c3[i][j] += lr * (net->state3[i][j].dw_sum / (float)batchSize);
            net->state3[i][j].dw_sum = 0;
        }
        // バイアス
        net->c3[VALUE_HIDDEN_UNITS2][j] += lr * (net->state3[VALUE_HIDDEN_UNITS2][j].dw_sum / (float)batchSize);
        net->state3[VALUE_HIDDEN_UNITS2][j].dw_sum = 0;
    }
}

float Predict(NNet *net, const uint16 features[])
{
    return forward(net, features, 0) - 0.5f;
}

double trainBatch(NNet *net, FeatureRecord *inputs[BATCH_SIZE], int batchSize)
{
    int i;
    float output;
    double loss = 0;
    float teacher;
    for (i = 0; i < batchSize; i++)
    {
        // 教師信号（-64~64 -> 0~1)
        teacher = (inputs[i]->stoneDiff + 64.0f) / 128.0f;
        output = forward(net, inputs[i]->featStats, 1);
        backward(net, inputs[i]->featStats, output, teacher);
        loss += fabs((teacher * 128 - 64) - (output * 128 - 64));
    }
    update_weights(net, batchSize);
    return loss / batchSize;
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

void Train(NNet net[NB_PHASE], FeatureRecord *gameRecords, size_t nbRecords)
{
    double loss;
    int i, phase, batchIdx;
    int inputSize[NB_PHASE] = {0, 0, 0, 0};
    int tmpSize[NB_PHASE] = {0, 0, 0, 0};
    FeatureRecord **inputs[NB_PHASE];
    FeatureRecord *batchInput[BATCH_SIZE];
    for (i = 0; i < nbRecords; i++)
    {
        inputSize[PHASE(gameRecords[i].nbEmpty)]++;
    }

    for (phase = 0; phase < NB_PHASE; phase++)
    {
        inputs[phase] = (FeatureRecord **)malloc(sizeof(FeatureRecord *) * inputSize[phase]);
    }

    // レコードをフェーズごとに振り分け
    for (i = 0; i < nbRecords; i++)
    {
        phase = PHASE(gameRecords[i].nbEmpty);
        inputs[phase][tmpSize[phase]] = &gameRecords[i];
        tmpSize[phase]++;
    }

    // すべてのフェーズに対して学習
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        loss = 0;
        // ミニバッチ学習
        for (batchIdx = 0; batchIdx < inputSize[phase]; batchIdx += BATCH_SIZE)
        {
            printf("NN train phase:%d batch:%d      \r", phase, batchIdx / BATCH_SIZE);
            sampling(inputs[phase], batchInput, inputSize[phase], BATCH_SIZE);
            loss += trainBatch(&net[phase], batchInput, BATCH_SIZE);
        }
        printf("NN Loss phase%d: %.3f          \n", phase, loss / (batchIdx / BATCH_SIZE));
    }
}

void SaveNets(NNet net[NB_PHASE], const char *file)
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
        writed += fwrite(&net[phase].c1, sizeof(float), (FEAT_NB_COMBINATION + 1) * VALUE_HIDDEN_UNITS1, fp);
        writed += fwrite(&net[phase].c2, sizeof(float), (VALUE_HIDDEN_UNITS1 + 1) * VALUE_HIDDEN_UNITS2, fp);
        writed += fwrite(&net[phase].c3, sizeof(float), (VALUE_HIDDEN_UNITS2 + 1), fp);
        if (writed < 3)
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

void LoadNets(NNet net[NB_PHASE], const char *file)
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
        readed += fread(&net[phase].c1, sizeof(float), (FEAT_NB_COMBINATION + 1) * VALUE_HIDDEN_UNITS1, fp);
        readed += fread(&net[phase].c2, sizeof(float), (VALUE_HIDDEN_UNITS1 + 1) * VALUE_HIDDEN_UNITS2, fp);
        readed += fread(&net[phase].c3, sizeof(float), (VALUE_HIDDEN_UNITS2 + 1), fp);
        if (readed < 3)
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