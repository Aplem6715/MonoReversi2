/**
 * @file regression.c
 * @author Daichi Sato
 * @brief 線形回帰による最終石差予測
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 * 各パターンの評価値を変数とし，それらを足し合わせることで最終石差を予測する。
 * 
 * M-buroさんの論文を参考に作成
 * https://skatgame.net/mburo/ps/improve.pdf
 * 
 */

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
            regr[phase].weight[0][feat] = (double *)calloc(FeatMaxIndex[feat], sizeof(double));
            regr[phase].weight[1][feat] = (double *)calloc(FeatMaxIndex[feat], sizeof(double));
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

void RegrCopyWeight(Regressor src[NB_PHASE], Regressor dst[NB_PHASE])
{
    int phase;
    int feat;
    uint32_t i;
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        for (feat = 0; feat < FEAT_NUM; feat++)
        {
            for (i = 0; i < FeatMaxIndex[feat]; i++)
            {
                dst[phase].weight[0][feat][i] = src[phase].weight[0][feat][i];
                dst[phase].weight[1][feat][i] = src[phase].weight[1][feat][i];
            }
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

double RegrPred(Regressor *regr, const uint16_t features[FEAT_NUM], uint8 player)
{
    int feat;
    double score = 0;

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
            writed += fwrite(regr[phase].weight[0][feat], sizeof(double), FeatMaxIndex[feat], fp);
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
            readed += fread(regr[phase].weight[0][feat], sizeof(double), FeatMaxIndex[feat], fp);
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