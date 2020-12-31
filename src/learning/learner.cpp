#include "model.h"
#include "../board.h"
#include "../bit_operation.h"
#include "../search/search.h"
#include <string>
#include <assert.h>
#include <random>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <direct.h>

using namespace std;

// 初期ランダム手数
static const uint8 TRAIN_RANDOM_TURNS = 4;
// ランダム行動割合（１試合に1回くらい）
static const double TRAIN_RANDOM_RATIO = 1.0 / 60.0;

// 学習結果確認の対戦回数（１色で１００試合＝全体で２００）
static const int TRAIN_NB_VERSUS = 100;
static const uint8 VERSUS_RANDOM_TURNS = 8;

static const int nbTrainCycles = 1000;
static const int nbGameOneCycle = 1024;
static const string modelFolder = "resources/model/";
static const string modelName = "model_";
static const string logFileName = "log/self_play.log";

static random_device rnd;
static mt19937 mt(rnd());
static uniform_real_distribution<double> rnd_prob(0.0, 1.0);

bool PlayOneGame(vector<GameRecord> &gameRecords, SearchTree tree[2], uint8 randomTurns, double randMoveRatio, bool useRecording)
{
    uint64 input, flip;
    int nbEmpty = 60;
    Board board;
    Evaluator eval[1];
    GameRecord record;

    board.Reset();
    InitEval(eval, board.GetBlack(), board.GetWhite());
    while (!board.IsFinished())
    {
        //board.Draw();
        // 置ける場所がなかったらスキップ
        if (board.GetMobility() == 0)
        {
            UpdateEvalPass(eval);
            board.Skip();
            continue;
        }

        // 指定ターン以内 or ランダム行動確率で
        if ((nbEmpty >= 60 - randomTurns) || rnd_prob(mt) < randMoveRatio)
        {
            // ランダム着手位置
            input = board.GetRandomPosMoveable();
        }
        else
        {
            // AIが着手位置を決める
            input = Search(&tree[board.GetTurnColor()], board.GetOwn(), board.GetOpp());
        }

        // 入力位置のインデックスを取得
        uint8 posIdx = CalcPosIndex(input);
        // 合法手判定
        assert(board.IsLegal(input));

        // 実際に着手
        flip = board.Put(input);
        nbEmpty--;
        UpdateEval(eval, posIdx, flip);

        if (useRecording)
        {
            // レコード設定
            for (int featIdx = 0; featIdx < FEAT_NUM; featIdx++)
            {
                record.featStats[featIdx] = eval->FeatureStates[featIdx];
            }
            record.nbEmpty = nbEmpty;
            record.resultForBlack = RESULT_UNSETTLED;
            record.color = board.GetTurnColor();

            // レコードを追加
            gameRecords.push_back(record);
        }

    } //end of loop:　while (!board.IsFinished())

    // 勝敗を表示
    board.Draw();
    int numBlack = board.GetStoneCount(Const::BLACK);
    int numWhite = board.GetStoneCount(Const::WHITE);
    signed char stoneDiff = numBlack - numWhite;

    if (useRecording)
    {
        // レコード終端から，リザルトが未確定のレコードに対してリザルトを設定
        for (auto i = gameRecords.end(); i->resultForBlack == RESULT_UNSETTLED; i--)
        {
            i->resultForBlack = stoneDiff;
        }
    }

    return numBlack > numWhite;
}

void SelfPlay(uint8 searchDepth)
{
    vector<GameRecord> gameRecords;
    SearchTree trees[2];
    ValueModel prevModel;
    ValueModel trainModel;
    ofstream logFile(logFileName);
    int winCount;
    double winRatio;

    InitTree(&trees[0], searchDepth, 100, 1, 6, &trainModel);
    InitTree(&trees[1], searchDepth, 100, 1, 6, &prevModel);

    int nbCycles = 0;
    while (true)
    {
        // 指定回数分の試合を実行
        for (int gameCnt = 0; gameCnt < nbGameOneCycle; gameCnt++)
        {
            cout << "Cycle: " << nbCycles << "\tPlaying: " << gameCnt << "\n";
            PlayOneGame(gameRecords, trees, TRAIN_RANDOM_TURNS, TRAIN_RANDOM_RATIO, true);
        }

        // 試合結果から学習
        trainModel.train(gameRecords);

        // 新旧モデルで対戦
        winCount = 0;
        for (int nbVS = 0; nbVS < TRAIN_NB_VERSUS; nbVS++)
        {
            if (PlayOneGame(gameRecords, trees, VERSUS_RANDOM_TURNS, 0, false))
            {
                // 勝ったら加算
                winCount++;
            }
        }

        // 対戦結果を保存
        winRatio = winCount / (double)TRAIN_NB_VERSUS;
        logFile << "VS Result " << nbCycles << "\t Win Ratio:" << setprecision(2) << winRatio * 100 << "%\t";

        // 対戦結果に応じてモデルを更新
        if (winRatio >= 0.55)
        {
            string modelDir = modelFolder + modelName + to_string(nbCycles) + "/";
            _mkdir(modelDir.c_str());

            trainModel.Save(modelDir);
            prevModel.Load(modelDir);
            logFile << "Model Updated!!!";
        }

        nbCycles++;
        logFile << "\n";
    }
}

int main()
{
    SelfPlay(4);
}