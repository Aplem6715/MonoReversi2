/**
 * @file nnet_trainer.cpp
 * @author Daichi Sato
 * @brief ニューラルネットの学習機能の定義
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 * ニューラルネットの学習を行う。
 * データ全体からいくつかのデータをランダムサンプリングして学習に利用する。
 * 誤差逆伝播，バッチ学習，活性化関数はtanhExp，誤差関数はmse
 * 
 */

#include "nnet_trainer.hpp"

extern "C"
{
#include "../ai/nnet.h"
#include "../ai/eval.h"
}

#include <assert.h>

#define _USE_MATH_DEFINES
#include <math.h>

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

float d_mse(float y, float t)
{
    return (y - t);
}

void InitWeight(NNet *net)
{
    int i, j, phase;
    srand(GLOBAL_SEED);
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
            shift += FTYPE_INDEX_MAX[FeatID2Type[featIdx]];

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