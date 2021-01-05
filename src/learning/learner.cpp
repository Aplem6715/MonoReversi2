
#define _CRT_SECURE_NO_WARNINGS

#include "../board.h"
#include "../bit_operation.h"
#include "../search/search.h"
#include "../game.h"
#include "nnet.h"
#include <signal.h>
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

// 学習結果確認の対戦回数（１色で50試合＝全体で100）
static const int TRAIN_NB_VERSUS = 50;
static const uint8 VERSUS_RANDOM_TURNS = 8;

static const int nbTrainCycles = 1000;
static const int nbGameOneCycle = 256; //1024;
#ifdef USE_NN
static const string modelFolder = "resources/model/";
static const string modelName = "model_";
#elif USE_REGRESSION
static const string modelFolder = "resources/regressor/";
static const string modelName = "regr_";
#endif
static const string logFileName = "log/self_play.log";
static const string testRecordDir = "./resources/record/WTH_7789/WTH_1982.wtb";
static const int nbTest = 100;

static random_device rnd;
static mt19937 mt(rnd());
static uniform_real_distribution<double> rnd_prob(0.0, 1.0);

volatile sig_atomic_t aborted = false;

void sig_handler(int sig)
{
    printf("Handle!!!!!!!!!!!!!!!!!!!!!!!!!!");
    aborted = true;
}

void ConverWthor2Feat(vector<FeatureRecord> &featRecords, WthorWTB &wthor);

bool PlayOneGame(vector<FeatureRecord> &featRecords, SearchTree tree[2], uint8 colorSwapped, uint8 randomTurns, double randMoveRatio, bool useRecording)
{
    uint64 input, flip;
    int nbEmpty = 60;
    Board board;
    Evaluator eval[2];
    FeatureRecord record;
    size_t topOfAddition = featRecords.size();

    board.Reset();
    ReloadEval(&eval[0], board.GetBlack(), board.GetWhite(), 1);
    ReloadEval(&eval[1], board.GetWhite(), board.GetBlack(), 0);
    while (!board.IsFinished())
    {
        //board.Draw();
        // 置ける場所がなかったらスキップ
        if (board.GetMobility() == 0)
        {
            UpdateEvalPass(&eval[0]);
            UpdateEvalPass(&eval[1]);
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
            input = Search(&tree[board.GetTurnColor() ^ colorSwapped], board.GetOwn(), board.GetOpp());
        }

        // 入力位置のインデックスを取得
        uint8 posIdx = CalcPosIndex(input);
        // 合法手判定
        assert(board.IsLegal(input));

        // 実際に着手
        flip = board.Put(input);
        nbEmpty--;
        UpdateEval(&eval[0], posIdx, flip);
        UpdateEval(&eval[1], posIdx, flip);

        if (useRecording)
        {
            // レコード設定
            for (int featIdx = 0; featIdx < FEAT_NUM; featIdx++)
            {
                record.featStats[featIdx] = eval[0].FeatureStates[featIdx];
            }
            record.nbEmpty = nbEmpty;
            record.stoneDiff = 1;
            record.own = board.GetBlack();
            record.opp = board.GetWhite();
            record.color = Const::BLACK;
            // レコードを追加
            featRecords.push_back(record);

            // レコード設定
            for (int featIdx = 0; featIdx < FEAT_NUM; featIdx++)
            {
                record.featStats[featIdx] = eval[1].FeatureStates[featIdx];
            }
            record.nbEmpty = nbEmpty;
            record.stoneDiff = -1;
            record.own = board.GetWhite();
            record.opp = board.GetBlack();
            record.color = Const::WHITE;

            // レコードを追加
            featRecords.push_back(record);
        }

    } //end of loop:　while (!board.IsFinished())

    // 勝敗を表示
    //board.Draw();
    int numBlack = board.GetStoneCount(Const::BLACK);
    int numWhite = board.GetStoneCount(Const::WHITE);
    signed char stoneDiff = numBlack - numWhite;

    if (useRecording)
    {
        // レコード終端から，リザルトが未確定のレコードに対してリザルトを設定
        for (; topOfAddition < featRecords.size(); topOfAddition++)
        {
            featRecords[topOfAddition].stoneDiff *= stoneDiff;
        }
    }

    return numBlack > numWhite;
}

void SelfPlay(uint8 searchDepth)
{

    SearchTree trees[2];
    InitTree(&trees[0], searchDepth, 100, 1, 6); // 旧
    InitTree(&trees[1], searchDepth, 100, 1, 6); // 新
#ifdef USE_NN
    InitWeight(trees[0].eval->net);
    InitWeight(trees[1].eval->net);
#elif USE_REGRESSION
    InitRegressor(trees[0].eval->regr);
    InitRegressor(trees[1].eval->regr);
#endif

    vector<FeatureRecord> featRecords;
    ofstream logFile;
    int winCount;
    double winRatio;
    int nbCycles = 0;
    logFile.open(logFileName, ios::app);
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

    for (nbCycles = 0; nbCycles < nbTrainCycles; nbCycles++)
    {
        // 指定回数分の試合を実行
        for (int gameCnt = 0; gameCnt < nbGameOneCycle; gameCnt++)
        {
            cout << "Cycle: " << nbCycles << "\tPlaying: " << gameCnt << "\r";
            PlayOneGame(featRecords, trees, 0, TRAIN_RANDOM_TURNS, TRAIN_RANDOM_RATIO, true);
        }

        // 試合結果から学習
#ifdef USE_NN
        Train(trees[1].eval->net, featRecords.data(), featRecords.size());
#elif USE_REGRESSION
        TrainRegressor(trees[1].eval->regr, featRecords.data(), testRecords.data(), featRecords.size(), testRecords.size());
#endif

        // 新旧モデルで対戦
        winCount = 0;
        for (int nbVS = 0; nbVS < TRAIN_NB_VERSUS; nbVS++)
        {
            if (PlayOneGame(featRecords, trees, 0, VERSUS_RANDOM_TURNS, 0, false))
            {
                // 勝ったら加算
                winCount++;
            }
        }
        // 白黒入れ替えて
        for (int nbVS = 0; nbVS < TRAIN_NB_VERSUS; nbVS++)
        {
            if (!PlayOneGame(featRecords, trees, 1, VERSUS_RANDOM_TURNS, 0, false))
            {
                // 勝ったら加算
                winCount++;
            }
        }

        // 対戦結果を保存
        winRatio = winCount / (double)(2 * TRAIN_NB_VERSUS);
        logFile << "VS Result " << nbCycles << "\t Win Ratio:" << setprecision(4) << winRatio * 100 << "%\t";
        cout << "VS Result " << nbCycles << "\t Win Ratio:" << setprecision(4) << winRatio * 100 << "%\t";

        // 対戦結果に応じてモデルを更新
        if (winRatio >= 0.60)
        {
            string modelDir = modelFolder + modelName + to_string(nbCycles) + "/";
            _mkdir(modelDir.c_str());

#ifdef USE_NN
            SaveNets(trees[1].eval->net, modelDir.c_str());
            // 旧ツリーに新Weightを上書きコピー
            for (int phase = 0; phase < NB_PHASE; phase++)
            {
                trees[0].eval->net[phase] = trees[1].eval->net[phase];
            }
#elif USE_REGRESSION
            SaveRegressor(trees[1].eval->regr, modelDir.c_str());
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
}

uint8 move88ToIndex(uint8 move88)
{
    return move88 / 10 - 1 + (move88 % 10 - 1) * 8;
}

void ConverWthor2Feat(vector<FeatureRecord> &featRecords, WthorWTB &wthor)
{
    uint64 input, flip;
    int nbEmpty = 60;
    int nbMoves = 0;
    Board board;
    Evaluator eval[2];
    FeatureRecord record;
    size_t topOfAddition = featRecords.size();

    board.Reset();
    ReloadEval(&eval[0], board.GetBlack(), board.GetWhite(), 1);
    ReloadEval(&eval[1], board.GetWhite(), board.GetBlack(), 0);
    while (!board.IsFinished())
    {
        //board.Draw();
        // 置ける場所がなかったらスキップ
        if (board.GetMobility() == 0)
        {
            UpdateEvalPass(&eval[0]);
            UpdateEvalPass(&eval[1]);
            board.Skip();
            continue;
        }

        // 入力位置のインデックスを取得
        uint8 posIdx = move88ToIndex(wthor.moves88[nbMoves]);
        nbMoves++;
        input = CalcPosBit(posIdx);
        // 合法手判定
        assert(board.IsLegal(input));

        // 実際に着手
        flip = board.Put(input);
        nbEmpty--;
        UpdateEval(&eval[0], posIdx, flip);
        UpdateEval(&eval[1], posIdx, flip);

        // 黒用レコード設定
        for (int featIdx = 0; featIdx < FEAT_NUM; featIdx++)
        {
            record.featStats[featIdx] = eval[0].FeatureStates[featIdx];
        }
        record.nbEmpty = nbEmpty;
        record.stoneDiff = 1;
        record.own = board.GetBlack();
        record.opp = board.GetWhite();
        record.color = Const::BLACK;
        // レコードを追加
        featRecords.push_back(record);

        // 白用レコード設定
        for (int featIdx = 0; featIdx < FEAT_NUM; featIdx++)
        {
            record.featStats[featIdx] = eval[1].FeatureStates[featIdx];
        }
        record.nbEmpty = nbEmpty;
        record.stoneDiff = -1;
        record.own = board.GetWhite();
        record.opp = board.GetBlack();
        record.color = Const::WHITE;

        // レコードを追加
        featRecords.push_back(record);

    } //end of loop:　while (!board.IsFinished())

    // 勝敗を表示
    //board.Draw();
    int numBlack = board.GetStoneCount(Const::BLACK);
    int numWhite = board.GetStoneCount(Const::WHITE);
    signed char stoneDiff = numBlack - numWhite;

    // レコード終端から，リザルトが未確定のレコードに対してリザルトを設定
    for (; topOfAddition < featRecords.size(); topOfAddition++)
    {
        featRecords[topOfAddition].stoneDiff *= stoneDiff;
    }
}

void LearnFromRecords(Evaluator *eval, string recordFileName)
{
    vector<FeatureRecord> featRecords;
    vector<FeatureRecord> testRecords;
    WthorHeaderWTB header;
    WthorHeaderWTB testHeader;
    WthorWTB wthorData;

    uint32 loaded = 0;
    int cycles = 0;
    int converted;
    size_t readSuccsess;
    FILE *fp = fopen(recordFileName.c_str(), "rb");
    FILE *tfp = fopen(testRecordDir.c_str(), "rb");

    fread(&header, sizeof(WthorHeaderWTB), 1, fp);
    fread(&testHeader, sizeof(WthorHeaderWTB), 1, tfp);

    for (loaded = 0; loaded < nbTest; loaded++)
    {
        readSuccsess = fread(&wthorData, sizeof(wthorData), 1, tfp);
        if (readSuccsess < 1)
        {
            cout << "テストデータ不足\n";
        }
        ConverWthor2Feat(testRecords, wthorData);
    }

    while (loaded < header.nbRecords && !aborted)
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
        Train(eval->net, featRecords.data(), featRecords.size());
#elif USE_REGRESSION
        TrainRegressor(eval->regr, featRecords.data(), testRecords.data(), featRecords.size(), testRecords.size());
#endif
        cycles++;
    }

    fclose(fp);
    fclose(tfp);
}

int main()
{
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        printf("\ncan't catch SIGINT\n");
        return 0;
    }

    string recordDir = "./resources/record/WTH_2001-2015/";
    SearchTree tree;
    int startYear = 2001, endYear = 2001;
    int year;
    int epochs = 1;

    InitTree(&tree, 4, 100, 1, 6); // 旧
#ifdef USE_NN
    InitWeight(tree.eval->net);
#elif USE_REGRESSION
    InitRegressor(tree.eval->regr);
#endif

    for (int epoch = 0; epoch < epochs; epoch++)
    {
        for (year = startYear; year <= endYear; year++)
        {
            //SelfPlay(2);
            LearnFromRecords(tree.eval, recordDir + "WTH_" + to_string(year) + ".wtb");
            string modelDir = modelFolder + modelName + "epoch" + to_string(epoch) + "_" + to_string(year) + "/";
            _mkdir(modelDir.c_str());
#ifdef USE_NN
            SaveNets(tree.eval->net, modelDir.c_str());
#elif USE_REGRESSION
            SaveRegressor(tree.eval->regr, modelDir.c_str());
#endif
            if (aborted)
            {
                break;
            }
        }
        if (aborted)
        {
            break;
        }
    }

    Game game(PlayerEnum::HUMAN, PlayerEnum::AI);
    game.GetTree(Const::WHITE)->eval->regr = tree.eval->regr;
    game.Start();

    DeleteTree(&tree);
}