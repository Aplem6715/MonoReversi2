
#define _CRT_SECURE_NO_WARNINGS
#include "nnet.h"
#include "eval.h"

#define _USE_MATH_DEFINES
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#define BATCH_SIZE 32
#define WEIGHT_SEED 42
#define HE_COEFF 2.0f

static const float lrInit = 0.005f;

float Uniform()
{
    return ((float)rand() + 1.0f) / ((float)RAND_MAX + 2.0f);
}

float rand_normal(float mu, float sigma)
{
    float z = sqrtf(-2.0f * logf(Uniform())) * sinf(2.0f * (float)M_PI * Uniform());
    return mu + sigma * z;
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

float forward(NNet *net, const uint16_t features[FEAT_NUM], uint8 isTrain)
{

    uint32_t shift = 0;
    int unitIdx;
    int prevUnit;
    int featIdx;
    float bias, sum = 0;

    // 入力層 -> 中間層１
    for (unitIdx = 0; unitIdx < VALUE_HIDDEN_UNITS1; unitIdx++)
    {
        sum = 0;
        shift = 0;
        bias = net->c1[NB_FEAT_COMB][unitIdx];
        for (featIdx = 0; featIdx < FEAT_NUM; featIdx++)
        {
            sum += net->c1[shift + features[featIdx]][unitIdx] * 1;
            sum += bias;
            shift += FeatMaxIndex[featIdx];
        }
#ifdef LEARN_MODE
        if (isTrain)
        {
            net->state1[unitIdx].sumIn = sum;
        }
#endif // LEARN_MODE
        net->out1[unitIdx] = act(sum);
    }

    // 中間層1 -> 中間層2
    for (unitIdx = 0; unitIdx < VALUE_HIDDEN_UNITS2; unitIdx++)
    {
        sum = 0;
        bias = net->c2[VALUE_HIDDEN_UNITS1][unitIdx];
        for (prevUnit = 0; prevUnit < VALUE_HIDDEN_UNITS1; prevUnit++)
        {
            sum += net->c2[prevUnit][unitIdx] * net->out1[prevUnit];
            sum += bias;
        }
#ifdef LEARN_MODE
        if (isTrain)
        {
            net->state2[unitIdx].sumIn = sum;
        }
#endif // LEARN_MODE
        net->out2[unitIdx] = act(sum);
    }

    // 中間層2 -> 出力層
    for (unitIdx = 0; unitIdx < 1; unitIdx++)
    {
        sum = 0;
        bias = net->c3[VALUE_HIDDEN_UNITS2][unitIdx];
        for (prevUnit = 0; prevUnit < VALUE_HIDDEN_UNITS2; prevUnit++)
        {
            sum += net->c3[prevUnit][unitIdx] * net->out2[prevUnit];
            sum += bias;
        }
#ifdef LEARN_MODE
        if (isTrain)
        {
            net->state3[unitIdx].sumIn = sum;
        }
#endif // LEARN_MODE
        net->out3[0] = sum;
    }

    return net->out3[0];
}

float Predict(NNet *net, const uint16_t features[])
{
    return forward(net, features, 0) - 0.5f;
}

#ifdef LEARN_MODE

void InitWeight(NNet *net)
{
    int i, j, phase;
    srand(WEIGHT_SEED);
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        net[phase].lr = lrInit;
        for (j = 0; j < VALUE_HIDDEN_UNITS1; j++)
        {
            for (i = 0; i < NB_FEAT_COMB; i++)
            {
                net[phase].c1[i][j] = rand_normal(0, sqrtf(HE_COEFF / FEAT_NUM));
                net[phase].dw1[i][j] = 0;
            }
            // バイアス
            net[phase].c1[i][j] = 0;
            net[phase].dw1[i][j] = 0;
        }
        for (j = 0; j < VALUE_HIDDEN_UNITS2; j++)
        {
            for (i = 0; i < VALUE_HIDDEN_UNITS1; i++)
            {
                net[phase].c2[i][j] = rand_normal(0, sqrtf(HE_COEFF / VALUE_HIDDEN_UNITS1));
                net[phase].dw2[i][j] = 0;
            }
            // バイアス
            net[phase].c2[i][j] = 0;
            net[phase].dw2[i][j] = 0;
        }
        for (i = 0; i < VALUE_HIDDEN_UNITS2; i++)
        {
            net[phase].c3[i][0] = rand_normal(0, sqrtf(HE_COEFF / VALUE_HIDDEN_UNITS2));
            net[phase].dw3[i][0] = 0;
        }
        // バイアス
        net[phase].c3[i][0] = 0;
        net[phase].dw3[i][0] = 0;
    }
}

void DecreaseNNlr(NNet *net)
{
    int phase;

    for (phase = 0; phase < NB_PHASE; phase++)
    {
        net[phase].lr /= 2.0f;
    }
}

void backward(NNet *net, const uint16_t features[FEAT_NUM], float y, float t)
{
    UnitState *targetStat;
    int j;
    int i;
    int nextUnit;
    float delta_w_sum;

    assert(net->out3[0] == y);

    // 出力　-> 中間2
    {
        targetStat = &net->state3[0];
        targetStat->delta = -(net->out3[0] - t) * 1; // * d_act(targetStat->sum);間違い？だからコメントアウトしておく
        // バイアス
        net->dw3[VALUE_HIDDEN_UNITS2][0] += targetStat->delta;
        for (i = 0; i < VALUE_HIDDEN_UNITS2; i++)
        {
            net->dw3[i][0] += targetStat->delta * net->out2[i];
        }
    }

    // 中間2 -> 中間1
    for (j = 0; j < VALUE_HIDDEN_UNITS2; j++)
    {
        delta_w_sum = net->state3[0].delta * net->c3[j][0];
        targetStat = &net->state2[j];
        targetStat->delta = d_act(targetStat->sumIn) * delta_w_sum;
        // バイアス
        net->dw2[VALUE_HIDDEN_UNITS1][j] += targetStat->delta;
        for (i = 0; i < VALUE_HIDDEN_UNITS1; i++)
        {
            net->dw2[i][j] += targetStat->delta * net->out1[i]; //act(targetStat->sum);
        }
    }

    // 中間1 -> 入力
    uint32_t shift;
    uint16_t featIdx;
    for (j = 0; j < VALUE_HIDDEN_UNITS1; j++)
    {
        delta_w_sum = 0;
        for (nextUnit = 0; nextUnit < VALUE_HIDDEN_UNITS2; nextUnit++)
        {
            // [delta_out * w_j]
            delta_w_sum += net->state2[nextUnit].delta * net->c2[j][nextUnit];
        }

        shift = 0;
        targetStat = &net->state1[j];
        targetStat->delta = d_act(targetStat->sumIn) * delta_w_sum;
        // バイアス
        net->dw1[NB_FEAT_COMB][j] += targetStat->delta;
        for (featIdx = 0; featIdx < FEAT_NUM; featIdx++)
        {
            i = shift + features[featIdx];
            shift += FeatMaxIndex[featIdx];

            net->dw1[i][j] += targetStat->delta * 1;
        }
    }
}

void update_weights(NNet *net, int batchSize)
{
    int i, j;
    for (j = 0; j < VALUE_HIDDEN_UNITS1; j++)
    {
        for (i = 0; i < NB_FEAT_COMB; i++)
        {
            net->c1[i][j] += net->lr * (net->dw1[i][j] / (float)batchSize);
            net->dw1[i][j] = 0;
        }
        // バイアス
        net->c1[NB_FEAT_COMB][j] += net->lr * (net->dw1[NB_FEAT_COMB][j] / (float)batchSize);
        net->dw1[NB_FEAT_COMB][j] = 0;
    }
    for (j = 0; j < VALUE_HIDDEN_UNITS2; j++)
    {
        for (i = 0; i < VALUE_HIDDEN_UNITS1; i++)
        {
            net->c2[i][j] += net->lr * (net->dw2[i][j] / (float)batchSize);
            net->dw2[i][j] = 0;
        }
        // バイアス
        net->c2[VALUE_HIDDEN_UNITS1][j] += net->lr * (net->dw2[VALUE_HIDDEN_UNITS1][j] / (float)batchSize);
        net->dw2[VALUE_HIDDEN_UNITS1][j] = 0;
    }
    for (j = 0; j < 1; j++)
    {
        for (i = 0; i < VALUE_HIDDEN_UNITS2; i++)
        {
            net->c3[i][j] += net->lr * (net->dw3[i][j] / (float)batchSize);
            net->dw3[i][j] = 0;
        }
        // バイアス
        net->c3[VALUE_HIDDEN_UNITS2][j] += net->lr * (net->dw3[VALUE_HIDDEN_UNITS2][j] / (float)batchSize);
        net->dw3[VALUE_HIDDEN_UNITS2][j] = 0;
    }
}

void trainBatch(NNet *net, FeatureRecord *inputs[BATCH_SIZE], int batchSize, uint8 player)
{
    int i;
    float output;
    float teacher;
    for (i = 0; i < batchSize; i++)
    {
        // 教師信号（-64~64 -> 0~1)
        teacher = (inputs[i]->stoneDiff + 64.0f) / 128.0f;
        output = forward(net, inputs[i]->featStats[player], 1);
        backward(net, inputs[i]->featStats[player], output, teacher);
    }
    update_weights(net, batchSize);
}

float TrainNN(NNet *net, FeatureRecord *featRecords, FeatureRecord *testRecords, size_t nbRecords, size_t nbTests)
{
    double loss, totalLoss;
    float teacher, output;
    int i, phase, batchIdx, testCnt, totalCnt;

    int testSize[NB_PHASE] = {0};
    int testTmpSize[NB_PHASE] = {0};
    FeatureRecord **tests[NB_PHASE];

    vector<FeatureRecord *> inputs[NB_PHASE];
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
            printf("NN train phase:%d batch:%d      \r", phase, batchIdx / BATCH_SIZE);
            sampling(inputs[phase], batchInput, BATCH_SIZE);
            trainBatch(&net[phase], batchInput, BATCH_SIZE, OWN);
        }
        loss = 0;
        testCnt = 0;
        for (i = 0; i < testSize[phase]; i++)
        {
            teacher = (tests[phase][i]->stoneDiff + 64.0f) / 128.0f;
            output = Predict(&net[phase], tests[phase][i]->featStats[0]);
            loss += fabsf((teacher - output) * 128.0f - 64.0f);
            testCnt++;
            totalCnt++;
        }
        totalLoss += loss;
        printf("NN Loss phase%d: %.3f          \n", phase, loss / (float)testCnt);
    }
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        free(tests[phase]);
    }
    return (float)totalLoss / totalCnt;
}
#endif

void SaveNets(NNet *net, const char *file)
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
        writed += fwrite(&net[phase].c1, sizeof(float), (NB_FEAT_COMB + 1) * VALUE_HIDDEN_UNITS1, fp);
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

void LoadNets(NNet *net, const char *file)
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
        readed += fread(&net[phase].c1, sizeof(float), (NB_FEAT_COMB + 1) * VALUE_HIDDEN_UNITS1, fp);
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