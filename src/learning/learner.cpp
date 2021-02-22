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

// 初期ランダム手数
static const uint8 TRAIN_RANDOM_TURNS = 6;
// ランダム行動割合（１試合に1回くらい）
static const double TRAIN_RANDOM_RATIO = 0.0 / 60.0;
// 次善手割合(PVS探索-orderingで次善手が探索されない事が多々ある→かなり高めの設定)
static const double TRAIN_SECOND_RATIO = 4.0 / 60.0;

// 学習結果確認の対戦回数（１色で50試合＝全体で100）
static const int TRAIN_NB_VERSUS = 50;
static const uint8 VERSUS_RANDOM_TURNS = 6;

static const int nbTrainCycles = 4096;
#ifdef USE_NN
static const string modelFolder = "resources/model/";
static const string modelName = "model_";
static const int nbGameOneCycle = 512; //1024;
#elif USE_REGRESSION
static const string modelFolder = "resources/regressor/";
static const string modelName = "regrV3_";
static const int nbGameOneCycle = 256;
#endif
static const string selfPlayLogFileName = "log/self_play_bonus2.log";
static const string recordLearnLogFileName = "log/record_learn.log";
static const string testRecordDir = "./resources/record/WTH_7789/WTH_1982.wtb";
static const int nbTest = 100;

static random_device rnd;
static mt19937 mt(rnd());
static uniform_real_distribution<double> rnd_prob(0.0, 1.0);

void ConverWthor2Feat(vector<FeatureRecord> &featRecords, WthorWTB &wthor);

uint8 PlayOneGame(vector<FeatureRecord> &featRecords, SearchTree *treeBlack, SearchTree *treeWhite, uint8 randomTurns, double randMoveRatio, double secondMoveRatio, bool useRecording)
{
    uint64_t flip;
    uint8 pos;
    int nbEmpty = 60;
    bool useSecondMove;
    Board board[1];
    Evaluator eval[2];
    FeatureRecord record;
    size_t topOfAddition = featRecords.size();

    BoardReset(board);
    if (useRecording)
    {
        EvalReload(&eval[0], BoardGetBlack(board), BoardGetWhite(board), OWN);
        EvalReload(&eval[1], BoardGetWhite(board), BoardGetBlack(board), OPP);
    }
    while (!BoardIsFinished(board))
    {
        //BoardDraw(board);
        //_sleep(750);

        // 終盤探索は確定的に
        if (nbEmpty <= treeBlack->endDepth)
        {
            randMoveRatio = 0;
            secondMoveRatio = 0;
        }

        // 置ける場所がなかったらスキップ
        if (BoardGetMobility(board) == 0)
        {
            if (useRecording)
            {
                EvalUpdatePass(&eval[0]);
                EvalUpdatePass(&eval[1]);
            }
            BoardSkip(board);
            continue;
        }

        // 指定ターン以内 or ランダム行動確率で
        if ((nbEmpty >= 60 - randomTurns) || rnd_prob(mt) < randMoveRatio)
        {
            // ランダム着手位置
            pos = BoardGetRandomPosMoveable(board);
            //printf("Random!!!\n");
        }
        else
        {
            useSecondMove = rnd_prob(mt) < secondMoveRatio;
            if (BoardGetTurnColor(board) == BLACK)
            {
                // AIが着手位置を決める
                pos = Search(treeBlack, BoardGetOwn(board), BoardGetOpp(board), useSecondMove);
                //printf("\n%f\n", treeBlack->score);
            }
            else
            {
                // AIが着手位置を決める
                pos = Search(treeWhite, BoardGetOwn(board), BoardGetOpp(board), useSecondMove);
                //printf("\n%f\n", treeWhite->score);
            }
        }

        // 合法手判定
        assert(BoardIsLegalTT(board, pos));

        // 実際に着手
        flip = BoardPutTT(board, pos);
        nbEmpty--;

        if (useRecording)
        {
            EvalUpdate(&eval[0], pos, flip);
            EvalUpdate(&eval[1], pos, flip);
            // ランダムターン中の記録はなし
            if (60 - nbEmpty > randomTurns)
            {
                for (int featIdx = 0; featIdx < FEAT_NUM; featIdx++)
                {
                    record.featStats[OWN][featIdx] = eval[0].FeatureStates[featIdx];
                    record.featStats[OPP][featIdx] = OpponentIndex(eval[0].FeatureStates[featIdx], FeatDigits[featIdx]);
                }
                record.nbEmpty = nbEmpty;
                record.stoneDiff = 1;
                record.color = BLACK;
                // レコードを追加
                featRecords.push_back(record);
            }
        }

    } //end of loop:　while (!BoardIsFinished(board))

    // 勝敗を表示
    //BoardDraw(board);
    int numBlack = BoardGetStoneCount(board, BLACK);
    int numWhite = BoardGetStoneCount(board, WHITE);
    signed char stoneDiff = numBlack - numWhite;

    if (useRecording)
    {
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

    return numBlack > numWhite ? BLACK : WHITE;
}

void SelfPlay(uint8 midDepth, uint8 endDepth, bool resetWeight)
{

    SearchTree trees[2];
    InitTree(&trees[0], midDepth, endDepth, 4, 8, 1, 1, 0, 0); // 旧
    InitTree(&trees[1], midDepth, endDepth, 4, 8, 1, 1, 0, 0); // 新

#ifdef USE_NN
#elif USE_REGRESSION
    RegrTrainInit(trees[0].eval->regr);
    RegrTrainInit(trees[1].eval->regr);
#endif

    if (resetWeight)
    {
#ifdef USE_NN
        InitWeight(trees[0].eval->net);
        InitWeight(trees[1].eval->net);
#elif USE_REGRESSION
        RegrClearWeight(trees[0].eval->regr);
        RegrClearWeight(trees[1].eval->regr);
        printf("Weight Init(Reset)\n");
#endif
    }
    else
    {
#if USE_REGRESSION
        RegrInitBeta(trees[0].eval->regr);
        RegrInitBeta(trees[1].eval->regr);
#endif
    }

    vector<FeatureRecord> featRecords;
    vector<FeatureRecord> dummyRecords;
    int winCount;
    double winRatio;
    float loss;
    int nbCycles = 0;
    ofstream logFile;
    logFile.open(selfPlayLogFileName, ios::app);
    logFile.seekp(ios::beg);

    vector<FeatureRecord> testRecords;
    WthorHeaderWTB testHeader;
    WthorWTB wthorData;
    size_t readSuccsess;
    FILE *tfp = fopen(testRecordDir.c_str(), "rb");
    fread(&testHeader, sizeof(WthorHeaderWTB), 1, tfp);

    for (nbCycles = 0; nbCycles < nbTest; nbCycles++)
    {
        readSuccsess = fread(&wthorData, sizeof(wthorData), 1, tfp);
        if (readSuccsess < 1)
        {
            cout << "テストデータ不足\n";
        }
        ConverWthor2Feat(testRecords, wthorData);
    }

    // メインプレイ
    for (nbCycles = 0; nbCycles < nbTrainCycles; nbCycles++)
    {
        featRecords.clear();
        // 指定回数分の試合を実行
        for (int gameCnt = 0; gameCnt < nbGameOneCycle; gameCnt++)
        {
            cout << "Cycle: " << nbCycles << "\tPlaying: " << gameCnt << "\r";
            ResetTree(&trees[0]);
            PlayOneGame(featRecords, &trees[0], &trees[0], TRAIN_RANDOM_TURNS, TRAIN_RANDOM_RATIO, TRAIN_SECOND_RATIO, true);
        }

        // 試合結果から学習
#ifdef USE_NN
        loss = TrainNN(trees[1].eval->net, featRecords.data(), testRecords.data(), featRecords.size(), testRecords.size());
#elif USE_REGRESSION
        loss = RegrTrain(trees[1].eval->regr, featRecords, testRecords.data(), testRecords.size());
#endif

        // 新旧モデルで対戦
        winCount = 0;
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

        // 対戦結果を保存
        winRatio = winCount / (double)(2 * TRAIN_NB_VERSUS);
        logFile << "VS Result " << nbCycles
                << "\t Win Ratio:" << winRatio * 100 << "%\t "
                << "Loss: " << setprecision(6) << loss << "%\t";
        cout << "VS Result " << nbCycles
             << "\t Win Ratio:" << winRatio * 100 << "%\t "
             << "Loss: " << setprecision(6) << loss << "%\t";

        // 対戦結果に応じてモデルを更新
        if (winRatio >= 0.60)
        {
            string modelDir = modelFolder + modelName + to_string(nbCycles) + "_Loss" + to_string((int)(loss * 100)) + "/";
            _mkdir(modelDir.c_str());

#ifdef USE_NN
            SaveNets(trees[1].eval->net, modelDir.c_str());
            // 旧ツリーに新Weightを上書きコピー
            for (int phase = 0; phase < NB_PHASE; phase++)
            {
                trees[0].eval->net[phase] = trees[1].eval->net[phase];
            }
#elif USE_REGRESSION
            RegrSave(trees[1].eval->regr, modelDir.c_str());
            // 旧ツリーに新Weightを上書きコピー
            for (int phase = 0; phase < NB_PHASE; phase++)
            {
                trees[0].eval->regr[phase] = trees[1].eval->regr[phase];
            }
#endif
            logFile << "Model Updated!!!";
            cout << "Model Updated!!!";
        }

        cout << "\n";
        logFile << endl;
        logFile.flush();
    }
    logFile.close();
    DeleteTree(&trees[0]);
    DeleteTree(&trees[1]);
}

uint8 move88ToIndex(uint8 move88)
{
    return move88 / 10 - 1 + (move88 % 10 - 1) * 8;
}

void ConverWthor2Feat(vector<FeatureRecord> &featRecords, WthorWTB &wthor)
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

void LearnFromRecords(Evaluator *eval, string recordFileName)
{
    vector<FeatureRecord> featRecords;
    vector<FeatureRecord> testRecords;
    WthorHeaderWTB header;
    WthorHeaderWTB testHeader;
    WthorWTB wthorData;

    uint32_t loaded = 0;
    int cycles = 0;
    int converted;
    float loss;
    size_t readSuccsess;
    FILE *fp = fopen(recordFileName.c_str(), "rb");
    FILE *tfp = fopen(testRecordDir.c_str(), "rb");
    ofstream logFile;
    logFile.open(recordLearnLogFileName, ios::app);
    logFile.seekp(ios::beg);

    fread(&header, sizeof(WthorHeaderWTB), 1, fp);
    fread(&testHeader, sizeof(WthorHeaderWTB), 1, tfp);

    testRecords.reserve(testHeader.nbRecords);
    for (loaded = 0; loaded < nbTest; loaded++)
    {
        readSuccsess = fread(&wthorData, sizeof(wthorData), 1, tfp);
        if (readSuccsess < 1)
        {
            cout << "テストデータ不足\n";
        }
        ConverWthor2Feat(testRecords, wthorData);
    }

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
            ConverWthor2Feat(featRecords, wthorData);
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
    fclose(tfp);
}

int main(int argc, char **argv)
{
    HashInit();
    srand(GLOBAL_SEED);

    SelfPlay(4, 16, false);
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