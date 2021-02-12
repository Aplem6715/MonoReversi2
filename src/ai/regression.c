
#define _CRT_SECURE_NO_WARNINGS
#include "regression.h"
#include "eval.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "../board.h"

void InitRegr(Regressor regr[NB_PHASE])
{
    int phase;
    int feat;
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        for (feat = 0; feat < FEAT_NUM; feat++)
        {
            regr[phase].weight[0][feat] = (float *)calloc(FeatMaxIndex[feat], sizeof(float));
            regr[phase].weight[1][feat] = (float *)calloc(FeatMaxIndex[feat], sizeof(float));
        }
    }
}

void DelRegr(Regressor regr[NB_PHASE])
{
    int phase;
    int feat;
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        for (feat = 0; feat < FEAT_NUM; feat++)
        {
            free(regr[phase].weight[0][feat]);
            free(regr[phase].weight[1][feat]);
        }
    }
}

void RegrClearWeight(Regressor regr[NB_PHASE])
{
    int phase, feat;
    uint32_t i;
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        for (feat = 0; feat < FEAT_NUM; feat++)
        {
            for (i = 0; i < FeatMaxIndex[feat]; i++)
            {
                regr[phase].weight[0][feat][i] = 0;
                regr[phase].weight[1][feat][i] = 0;
            }
        }
    }
}

void RegrApplyWeightToOpp(Regressor *regr)
{
    uint8 feat;
    uint16_t i;
    for (feat = 0; feat < FEAT_NUM; feat++)
    {
        for (i = 0; i < FeatMaxIndex[feat]; i++)
        {
            regr->weight[1][feat][i] = regr->weight[0][feat][OpponentIndex(i, FeatDigits[feat])];
        }
    }
}

float RegrPred(Regressor *regr, const uint16_t features[FEAT_NUM], uint8 player)
{
    int feat;
    float score = 0;

    for (feat = 0; feat < FEAT_NUM; feat++)
    {
        score += regr->weight[player][feat][features[feat]];
        assert(features[feat] < FeatMaxIndex[feat]);
    }

#ifdef LEARN_MODE
    //assert(player == OWN);
#endif

    return score;
}

void RegrSave(Regressor regr[NB_PHASE], const char *file)
{
    int phase, feat;
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
        for (feat = 0; feat < FEAT_NUM; feat++)
        {
            writed += fwrite(regr[phase].weight[0][feat], sizeof(float), FeatMaxIndex[feat], fp);
        }
        if (writed < NB_FEAT_COMB)
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

void RegrLoad(Regressor regr[NB_PHASE], const char *file)
{
    int phase, feat;
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
        for (feat = 0; feat < FEAT_NUM; feat++)
        {
            readed += fread(regr[phase].weight[0][feat], sizeof(float), FeatMaxIndex[feat], fp);
        }

        RegrApplyWeightToOpp(&regr[phase]);
        if (readed < NB_FEAT_COMB)
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