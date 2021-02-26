/**
 * @file learner.cpp
 * @author Daichi Sato
 * @brief 評価関数の学習を行う
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 */

#define _CRT_SECURE_NO_WARNINGS

#ifdef USE_NN
#include "nnet_trainer.hpp"
#elif USE_REGRESSION
#include "regr_trainer.hpp"
#endif

#include "learn_const.hpp"
#include "selfplay.hpp"
#include "ascii_loader.hpp"

extern "C"
{
#include "../ai/eval.h"
#include "../const.h"
#include "../board.h"
#include "../bit_operation.h"
#include "../search/search.h"
#include "../game.h"
#include "../search/mid.h"
#include "../search/mpc.h"

#ifdef USE_NN
#include "../ai/nnet.h"
#elif USE_REGRESSION
#include "../ai/regression.h"
#endif
}

#include <signal.h>
#include <assert.h>

#include <string>
#include <random>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <direct.h>

using namespace std;

void ConvertAppendWthor2Feat(vector<FeatureRecord> &featRecords, WthorWTB &wthor);

uint8 move88ToIndex(uint8 move88)
{
    return move88 / 10 - 1 + (move88 % 10 - 1) * 8;
}

void ConvertAppendWthor2Feat(vector<FeatureRecord> &featRecords, WthorWTB &wthor)
{
    uint64_t flip;
    uint8 pos;
    int nbEmpty = 60;
    int nbMoves = 0;
    Board board[1];
    Evaluator eval[2];
    FeatureRecord record;
    size_t topOfAddition = featRecords.size();

    BoardReset(board);
    EvalReload(&eval[0], BoardGetBlack(board), BoardGetWhite(board), OWN);
    EvalReload(&eval[1], BoardGetWhite(board), BoardGetBlack(board), OPP);
    while (!BoardIsFinished(board))
    {
        //BoardDraw(board);
        // 置ける場所がなかったらスキップ
        if (BoardGetMobility(board) == 0)
        {
            EvalUpdatePass(&eval[0]);
            EvalUpdatePass(&eval[1]);
            BoardSkip(board);
            continue;
        }

        // 入力位置のインデックスを取得
        pos = move88ToIndex(wthor.moves88[nbMoves]);
        nbMoves++;

        // 合法手判定
        assert(BoardIsLegalTT(board, pos));

        // 実際に着手
        flip = BoardPutTT(board, pos);
        nbEmpty--;
        EvalUpdate(&eval[0], pos, flip);
        EvalUpdate(&eval[1], pos, flip);

        //黒用レコード設定
        for (int featIdx = 0; featIdx < FEAT_NUM; featIdx++)
        {
            record.featStats[OWN][featIdx] = eval[0].FeatureStates[featIdx];
            record.featStats[OPP][featIdx] = OpponentIndex(eval[0].FeatureStates[featIdx], FeatDigits[featIdx]);
            assert(record.featStats[OWN][featIdx] < FeatMaxIndex[featIdx]);
            assert(record.featStats[OPP][featIdx] < FeatMaxIndex[featIdx]);
        }
        record.nbEmpty = nbEmpty;
        record.stoneDiff = 1;
        record.color = BLACK;
        // レコードを追加
        featRecords.push_back(record);
        assert(CountBits(~(BoardGetBlack(board) | BoardGetWhite(board))) == nbEmpty);

    } //end of loop:　while (!BoardIsFinished(board))

    // 勝敗を表示
    //BoardDraw(board);
    int numBlack = BoardGetStoneCount(board, BLACK);
    int numWhite = BoardGetStoneCount(board, WHITE);
    signed char stoneDiff = numBlack - numWhite;

    // レコード終端から，リザルトが未確定のレコードに対してリザルトを設定
    for (; topOfAddition < featRecords.size(); topOfAddition++)
    {
        featRecords[topOfAddition].stoneDiff *= stoneDiff;
        if (numBlack > numWhite)
        {
            featRecords[topOfAddition].stoneDiff += WIN_BONUS;
        }
        else if (numWhite > numBlack)
        {
            featRecords[topOfAddition].stoneDiff -= WIN_BONUS;
        }
    }
}

void GetTestData(vector<FeatureRecord> &testRecords)
{
    WthorWTB wthorData;

    WthorHeaderWTB testHeader;
    FILE *tfp = fopen(testRecordDir.c_str(), "rb");
    fread(&testHeader, sizeof(WthorHeaderWTB), 1, tfp);

    testRecords.reserve(testHeader.nbRecords);
    uint32_t loaded = 0;
    size_t readSuccsess;
    for (loaded = 0; loaded < nbTest; loaded++)
    {
        readSuccsess = fread(&wthorData, sizeof(wthorData), 1, tfp);
        if (readSuccsess < 1)
        {
            cout << "テストデータ不足\n";
        }
        ConvertAppendWthor2Feat(testRecords, wthorData);
    }
    fclose(tfp);
}

void LearnFromRecords(Evaluator *eval, string recordFileName, vector<FeatureRecord> &testRecords)
{
    vector<FeatureRecord> featRecords;
    WthorHeaderWTB header;
    WthorWTB wthorData;

    uint32_t loaded = 0;
    int cycles = 0;
    int converted;
    float loss;
    size_t readSuccsess;
    FILE *fp = fopen(recordFileName.c_str(), "rb");
    ofstream logFile;
    logFile.open(recordLearnLogFileName, ios::app);
    logFile.seekp(ios::beg);

    fread(&header, sizeof(WthorHeaderWTB), 1, fp);

    featRecords.reserve(header.nbRecords);
    while (loaded < header.nbRecords)
    {
        featRecords.clear();
        for (converted = 0; converted < nbGameOneCycle; converted++)
        {
            // 1試合分のデータを読み込む
            readSuccsess = fread(&wthorData, sizeof(wthorData), 1, fp);
            if (readSuccsess < 1)
                break;

            loaded++;
            // データを変換
            ConvertAppendWthor2Feat(featRecords, wthorData);
        }
        cout << "Cycle: " << cycles << "  Loaded: " << loaded << endl;
        // 試合結果から学習
#ifdef USE_NN
        loss = TrainNN(eval->net, featRecords.data(), testRecords.data(), featRecords.size(), testRecords.size());
#elif USE_REGRESSION
        loss = RegrTrain(eval->regr, featRecords, testRecords.data(), testRecords.size());
#endif
        logFile << "Cycle: " << cycles << "\t "
                << "Loss: " << setprecision(6) << loss << "%" << endl;
        logFile.flush();
        cycles++;
    }

    logFile.close();
    fclose(fp);
}

double LearnFromAscii(SearchTree trees[2], string recordFileName, vector<FeatureRecord> &testRecords, int fileCnt)
{
    vector<FeatureRecord> featRecords;
    vector<FeatureRecord> dummyRecords;
    ConvertAscii2Feat(featRecords, recordFileName);
    printf("Done Loading Ascii Records:%lld\n", featRecords.size());

    const int epochMax = 1;
    uint64_t gamesCnt = 0;
    float loss;
    ofstream logFile;
    logFile.open(recordLearnLogFileName, ios::app);
    logFile.seekp(ios::beg);

    for (int epoch = 0; epoch < epochMax; epoch++)
    {
#ifdef USE_NN
        loss = TrainNN(eval->net, featRecords.data(), testRecords.data(), featRecords.size(), testRecords.size());
#elif USE_REGRESSION
        loss = RegrTrain(trees[1].eval->regr, featRecords, testRecords.data(), testRecords.size());
#endif
        gamesCnt += featRecords.size();
        logFile << "Epoch: " << epoch << "\t "
                << "Games: " << gamesCnt << "\t "
                << "Loss: " << setprecision(6) << loss << "%" << endl;
        logFile.flush();
    }

    // 新旧モデルで対戦
    unsigned int winCount = 0;
    for (int nbVS = 0; nbVS < TRAIN_NB_VERSUS; nbVS++)
    {
        ResetTree(&trees[0]);
        ResetTree(&trees[1]);
        // 新規ウェイトを黒にしてプレイ
        if (PlayOneGame(dummyRecords, &trees[1], &trees[0], VERSUS_RANDOM_TURNS, 0, 0, false) == BLACK)
        {
            // 勝ったら加算
            winCount++;
        }
    }
    // 白黒入れ替えて
    for (int nbVS = 0; nbVS < TRAIN_NB_VERSUS; nbVS++)
    {
        ResetTree(&trees[0]);
        ResetTree(&trees[1]);
        // 新規ウェイトを白にしてプレイ
        if (PlayOneGame(dummyRecords, &trees[0], &trees[1], VERSUS_RANDOM_TURNS, 0, 0, false) == WHITE)
        {
            // 勝ったら加算
            winCount++;
        }
    }

    double winRatio = winCount / (double)(2 * TRAIN_NB_VERSUS);
    logFile << "VS Result " << fileCnt
            << "\t Win Ratio:" << winRatio * 100 << "%\t "
            << "Loss: " << setprecision(6) << loss << "%\t";
    cout << "VS Result " << fileCnt
         << "\t Win Ratio:" << winRatio * 100 << "%\t "
         << "Loss: " << setprecision(6) << loss << "%\t";
    // 対戦結果に応じてモデルを更新
    if (winRatio >= 0.60)
    {
        string modelDir = modelFolder + modelName + "Ascii" + to_string(fileCnt) + "_Loss" + to_string((int)(loss * 100)) + "/";
        _mkdir(modelDir.c_str());
        RegrSave(trees[1].eval->regr, modelDir.c_str());
        // 旧ツリーに新Weightを上書きコピー
        for (int phase = 0; phase < NB_PHASE; phase++)
        {
            trees[0].eval->regr[phase] = trees[1].eval->regr[phase];
        }
        logFile << "Model Updated!!!";
        cout << "Model Updated!!!";
    }

    cout << "\n";
    logFile << endl;
    logFile.flush();

    logFile.close();
    return loss;
}

void LearnFromAsciiAllFileInDir(bool resetWeight, string folder)
{
    SearchTree trees[2];
    InitTree(&trees[0], 4, 10, 4, 8, 1, 1, 0, 0); // 旧
    InitTree(&trees[1], 4, 10, 4, 8, 1, 1, 0, 0); // 新

    RegrTrainInit(trees[0].eval->regr);
    RegrTrainInit(trees[1].eval->regr);
    if (resetWeight)
    {
        RegrClearWeight(trees[0].eval->regr);
        RegrClearWeight(trees[1].eval->regr);
        printf("Weight Init(Reset)\n");
    }
    else
    {
        RegrInitBeta(trees[0].eval->regr);
        RegrInitBeta(trees[1].eval->regr);
    }

    vector<FeatureRecord> testRecord;
    GetTestData(testRecord);

    const vector<string> files = {
        "csc112-1.log",
        "csc112-2.log",
        "csc113-1.log",
        "csc113-2.log",
        "csc116-1.log",
        "csc116-2.log",
        "csc117-1.log",
        "csc117-2.log",
        "csc120-1.log",
        "csc120-2.log",
        "csc121-1.log",
        "csc121-2.log",
        "csc122-1.log",
        "csc122-2.log",
        "csc123-1.log",
        "csc123-2.log",
        "csc124-1.log",
        "csc124-2.log",
        "csc125-1.log",
        "csc125-2.log",
        "csc126-1.log",
        "csc126-2.log",
        "csc127-1.log",
        "csc127-2.log",
        "csc128-1.log",
        "csc128-2.log",
    };

    int fileCnt = 0;
    double loss;
    // 学習
    for (string file : files)
    {
        loss = LearnFromAscii(trees, folder + file, testRecord, fileCnt);
        fileCnt++;
    }

    string modelDir = modelFolder + modelName + "Ascii" + to_string(fileCnt) + "_Loss" + to_string((int)(loss * 100)) + "_Last/";
    _mkdir(modelDir.c_str());
    RegrSave(trees[1].eval->regr, modelDir.c_str());

    DeleteTree(&trees[0]);
    DeleteTree(&trees[1]);
}

int main(int argc, char **argv)
{
    HashInit();
    srand(GLOBAL_SEED);

    LearnFromAsciiAllFileInDir(true, "./resources/record/correctbk/");
    //SelfPlay(4, 16, false, testRecord);

    //MPCSampling(nbPlay, 6, 4.0 / 60.0, 1, idxShift);
    /*
    string recordDir = "./resources/record/";
    SearchTree tree;
    int startYear = 2001, endYear = 2015;
    int year;
    int epochs = 5;
    bool resetWeight = true;

    InitTree(&tree, 4, 6, 100, 1, 6);
#if USE_REGRESSION
    RegrTrainInit(tree.eval->regr);
#endif

    /*
    Board board[1];
    uint64_t black, white;
    while (true)
    {
        black = 18014398509481984ULL;
        white = 35184372088832ULL;
        board.Draw(black, white, 0);
        EvalReload(tree.eval, black, white, 0);
        tree.eval->nbEmpty = 32;
        float val = EvalNNet(tree.eval);
        printf("val: %f\n", val);
    }*/
    /*
    if (resetWeight)
    {
#ifdef USE_NN
        InitWeight(tree.eval->net);
#elif USE_REGRESSION
        RegrClearWeight(tree.eval->regr);
#endif
    }
    for (int epoch = 0; epoch < epochs; epoch++)
    {
        */
    /*
        for (year = startYear; year <= endYear; year++)
        {
            LearnFromRecords(tree.eval, recordDir + "WTH_2001-2015/WTH_" + to_string(year) + ".wtb");
        }*/
    /*
        LearnFromRecords(tree.eval, recordDir + "GGS/GGS_145-154.wtb");

        string modelDir = modelFolder + modelName + "epoch" + to_string(epoch) + "/";
        _mkdir(modelDir.c_str());
#ifdef USE_NN
        SaveNets(tree.eval->net, modelDir.c_str());
#elif USE_REGRESSION
        RegrSave(tree.eval->regr, modelDir.c_str());
#endif

#ifdef USE_NN
        DecreaseNNlr(tree.eval->net);
#elif USE_REGRESSION
        RegrDecreaseBeta(tree.eval->regr, 0.75f);
#endif
    }

    int phase;
    Game game[1];
    GameInit(game, PlayerEnum::HUMAN, PlayerEnum::AI);
// TODO pointer dainyu matigai
#ifdef USE_NN
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        game.GetTree(WHITE)->eval->net[phase] = tree.eval->net[phase];
    }
#elif USE_REGRESSION
    for (phase = 0; phase < NB_PHASE; phase++)
    {
        game.GetTree(WHITE)->eval->regr[phase] = tree.eval->regr[phase];
    }
#endif
    game.Start();

    DeleteTree(&tree);*/
}