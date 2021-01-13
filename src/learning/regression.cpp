
#define _CRT_SECURE_NO_WARNINGS
#include "regression.h"
#include "../ai/eval.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "../board.h"

#define BATCH_SIZE 128
static const float BETA_INIT = 0.005f;

static const uint32 FeatTypeMaxIndex[] = {
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
    POW3_8,  // BMRAN  26244
};

static const uint16 FeatTypeNbRots[] = {
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
    4, // BMRAN
};

static uint16 SAME_INDEX_8[POW3_8];
static uint16 SAME_INDEX_7[POW3_7];
static uint16 SAME_INDEX_6[POW3_6];
static uint16 SAME_INDEX_5[POW3_5];
static uint16 SAME_INDEX_4[POW3_4];
static uint16 SAME_INDEX_EDGE[POW3_10];
static uint16 SAME_INDEX_CORNR[POW3_9];
static uint16 SAME_INDEX_BMRAN[POW3_8];

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

void InitRegrBeta(Regressor regr[NB_PHASE])
{
    int phase;
    for (phase = 0; phase < NB_PHASE; phase++)
    {
#ifdef LEARN_MODE
        regr[phase].beta = BETA_INIT;
#endif
    }
}

void ClearRegressorWeight(Regressor regr[NB_PHASE])
{
    int phase, feat;
    uint32 i;
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
    uint16 i;
    for (feat = 0; feat < FEAT_NUM; feat++)
    {
        for (i = 0; i < FeatMaxIndex[feat]; i++)
        {
            regr->weight[1][feat][i] = regr->weight[0][feat][OpponentIndex(i, FeatDigits[feat])];
        }
    }
}

float PredRegressor(Regressor *regr, const uint16 features[FEAT_NUM], uint8 player)
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

#ifdef LEARN_MODE

void DecreaseRegrBeta(Regressor regr[NB_PHASE], float mul)
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
    float alpha;
    uint16 idx, sameIdx, featIdx;

    uint32 appearSum;
    float delSum;

    // タイプごとの対象型インデックスへの参照を配列として持っておく
    uint16 *FeatTypeSames[NB_FEATURE_TYPES] = {
        SAME_INDEX_8, //LINE2
        SAME_INDEX_8, //LINE3
        SAME_INDEX_8, //LINE4
        SAME_INDEX_4, //DIAG4
        SAME_INDEX_5, //DIAG5
        SAME_INDEX_6, //DIAG6
        SAME_INDEX_7, //DIAG7
        SAME_INDEX_8, // DIAG8
        SAME_INDEX_EDGE,
        SAME_INDEX_CORNR,
        SAME_INDEX_BMRAN,
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
            sameIdx = FeatTypeSames[featType][idx];

            // 同タイプ，同インデックス，対象型の出現回数と誤差の合計を求める
            for (rot = 0; rot < FeatTypeNbRots[featType]; rot++)
            {
                featIdx = feat + rot;

                appearSum += regr->nbAppears[featIdx][idx];
                delSum += regr->del[featIdx][idx];
                // 対象型の分も加算
                appearSum += regr->nbAppears[featIdx][sameIdx];
                delSum += regr->del[featIdx][sameIdx];
            }

            // 同タイプ，同インデックス，対象型のウェイトを更新
            for (rot = 0; rot < FeatTypeNbRots[featType]; rot++)
            {
                alpha = fminf(regr->beta / 50.0f, regr->beta / (float)appearSum);
                // ウェイト調整
                regr->weight[0][feat + rot][idx] += alpha * delSum;
                // 対象型についても調整
                regr->weight[0][feat + rot][sameIdx] += alpha * delSum;
            }
        }
        feat += FeatTypeNbRots[featType];
    }
}

void ResetRegrState(Regressor *regr)
{
    uint16 featIdx, i;
    for (featIdx = 0; featIdx < FEAT_NUM; featIdx++)
    {
        for (i = 0; i < FeatMaxIndex[featIdx]; i++)
        {
            regr->nbAppears[featIdx][i] = 0;
            regr->del[featIdx][i] = 0;
        }
    }
}

void CalcWeightDelta(Regressor *regr, const uint16 features[FEAT_NUM], float error)
{
    int featType;

    for (featType = 0; featType < FEAT_NUM; featType++)
    {
        regr->nbAppears[featType][features[featType]]++;
        regr->del[featType][features[featType]] += error;
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
        output = PredRegressor(regr, inputs[i]->featStats[OWN], OWN);
        CalcWeightDelta(regr, inputs[i]->featStats[OWN], teacher - output);

        // 反転パターンも学習
        output = PredRegressor(regr, inputs[i]->featStats[OPP], OWN); // ※featの方を反転しているのでplayerは0に
        CalcWeightDelta(regr, inputs[i]->featStats[OPP], (-teacher) - output);
        if (debug)
        {
            //Board::Draw(inputs[i]->own, inputs[i]->opp, 0);
            printf("Color: %d,  Score: %f, Pred: %f\n", inputs[i]->color, teacher, output);
        }
    }
    UpdateRegrWeights(regr);
    RegrApplyWeightToOpp(regr);
}

#ifdef LEARN_MODE
void InitRegrTrain(Regressor regr[NB_PHASE])
{
    int phase, i;

    static uint16 *SIMPLE_SAMES[] = {
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
            regr[phase].nbAppears[featIdx] = (uint32 *)malloc(sizeof(uint32) * FeatMaxIndex[featIdx]);
            regr[phase].del[featIdx] = (float *)malloc(sizeof(float) * FeatMaxIndex[featIdx]);
        }
    }

    uint16 same, idx;
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
    static uint16 corn_revs[9] = {POW3_0, POW3_3, POW3_6, POW3_1, POW3_4, POW3_7, POW3_2, POW3_5, POW3_8};
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

    // BMRANの対称インデックスを計算
    static uint16 bmran_revs[8] = {POW3_0, POW3_4, POW3_6, POW3_7, POW3_1, POW3_5, POW3_2, POW3_3};
    for (i = 0; i < POW3_8; i++)
    {
        same = 0;
        idx = i;
        for (j = 0; j < 8; j++)
        {
            same += idx % 3 * bmran_revs[j];
            idx /= 3;
        }
        SAME_INDEX_BMRAN[i] = same;
    }

    // 連結EDGEの対称インデックスを計算
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
#endif

float TrainRegressor(Regressor regr[NB_PHASE], vector<FeatureRecord> &featRecords, FeatureRecord *testRecords, size_t nbTests)
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
        inputs[phase].reserve(featRecords.size() / 15 + 1);
        tests[phase] = (FeatureRecord **)malloc(sizeof(FeatureRecord *) * testSize[phase]);
    }

    // レコードをフェーズごとに振り分け
    for (i = 0; i < featRecords.size(); i++)
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
            loss += fabsf(tests[phase][i]->stoneDiff - PredRegressor(&regr[phase], tests[phase][i]->featStats[0], 0));
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
void LoadRegressor(Regressor regr[NB_PHASE], const char *file)
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