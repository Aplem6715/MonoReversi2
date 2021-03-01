
#include "selfplay.hpp"

#define _CRT_SECURE_NO_WARNINGS
#ifdef USE_NN
#include "nnet_trainer.hpp"
#elif USE_REGRESSION
#include "regr_trainer.hpp"
#endif

#include "learn_const.hpp"

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
        if ((nbEmpty >= 60 - randomTurns) || rnd_prob01(mt) < randMoveRatio)
        {
            // ランダム着手位置
            pos = BoardGetRandomPosMoveable(board);
            //printf("Random!!!\n");
        }
        else
        {
            useSecondMove = rnd_prob01(mt) < secondMoveRatio;
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
                    record.featStats[OPP][featIdx] = OpponentIndex(eval[0].FeatureStates[featIdx], FTYPE_DIGIT[FeatID2Type[featIdx]]);
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

void SelfPlay(uint8 midDepth, uint8 endDepth, bool resetWeight, vector<FeatureRecord> &testRecords)
{

    SearchTree trees[2];
    TreeInit(&trees[0]); // 旧
    TreeInit(&trees[1]); // 新
    TreeConfigDepth(&trees[0], midDepth, endDepth);
    TreeConfigDepth(&trees[1], midDepth, endDepth);

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
    double loss;
    int nbCycles = 0;
    ofstream logFile;
    logFile.open(selfPlayLogFileName, ios::app);
    logFile.seekp(ios::beg);

    // メインプレイ
    for (nbCycles = 0; nbCycles < nbTrainCycles; nbCycles++)
    {
        featRecords.clear();
        // 指定回数分の試合を実行
        for (int gameCnt = 0; gameCnt < nbGameOneCycle; gameCnt++)
        {
            cout << "Cycle: " << nbCycles << "\tPlaying: " << gameCnt << "\r";
            TreeReset(&trees[0]);
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
            TreeReset(&trees[0]);
            TreeReset(&trees[1]);
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
            TreeReset(&trees[0]);
            TreeReset(&trees[1]);
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
            RegrCopyWeight(trees[1].eval->regr, trees[0].eval->regr);
            /*
            for (int phase = 0; phase < NB_PHASE; phase++)
            {
                trees[0].eval->regr[phase] = trees[1].eval->regr[phase];
            }*/
#endif
            logFile << "Model Updated!!!";
            cout << "Model Updated!!!";
        }

        cout << "\n";
        logFile << endl;
        logFile.flush();
    }
    logFile.close();
    TreeDelete(&trees[0]);
    TreeDelete(&trees[1]);
}
