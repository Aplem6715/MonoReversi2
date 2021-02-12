
#define _CRT_SECURE_NO_WARNINGS
#include "nnet.h"
#include "eval.h"

#define _USE_MATH_DEFINES
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

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

float mse(float y, float t)
{
    float diff = t - y;
    return diff * diff / 2;
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