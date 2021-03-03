/**
 * @file mpc_playout.cpp
 * @author Daichi Sato
 * @brief 自己対戦を繰り返し，深度別に評価関数の誤差の統計を取得する。
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 */

#define _CRT_SECURE_NO_WARNINGS

extern "C"
{
#include "../board.h"
#include "../search/search.h"
#include "../game.h"
#include "../search/mid.h"
#include "../search/mpc.h"
}

#include <string>
#include <assert.h>
#include <random>
#include <fstream>
#include <iostream>
#include <time.h>

using namespace std;
static random_device rnd;
static mt19937 mt(rnd());
static uniform_real_distribution<double> rnd_prob01(0.0, 1.0);

void MPCSampling(int nbPlay, int randomTurns, double randMoveRatio, bool enableLog, int matchIdxShift, uint8 shallow, uint8 deep, uint8 minimum)
{
    SearchTree deepTree[1], shallowTree[1];
    Board board[1];
    uint8 pos;
    uint8 nbEmpty;
    FILE *logFile;
    int i;

    minimum = deep > minimum ? deep : minimum;
    // 中盤探索深度は後から設定される
    TreeInit(shallowTree);
    TreeInit(deepTree);

    // 深すぎる深度 = 超遅い 設定忘れを防ぐ
    TreeConfig(shallowTree, 100, minimum, 0, true, false, true);
    TreeConfig(deepTree, 100, minimum, 0, true, false, true);

    logFile = fopen(MPC_RAW_FILE, "a");

    for (i = 0; i < nbPlay; i++)
    {
        nbEmpty = 60;
        BoardReset(board);
        while (!BoardIsFinished(board))
        {
            if (nbEmpty <= deep)
            {
                break;
            }
            if (enableLog)
            {
                BoardDraw(board);
                //_sleep(500);
            }

            // 置ける場所がなかったらスキップ
            if (BoardGetMobility(board) == 0)
            {
                BoardSkip(board);
                continue;
            }

            // 着手
            if (nbEmpty > deepTree->option.endDepth && ((nbEmpty >= 60 - randomTurns) || rnd_prob01(mt) < randMoveRatio))
            {
                // ランダム着手位置
                pos = BoardGetRandomPosMoveable(board);
                //printf("Random!!!\n");
            }
            else
            {
                SearchSetup(deepTree, BoardGetOwn(board), BoardGetOpp(board));
                SearchSetup(shallowTree, BoardGetOwn(board), BoardGetOpp(board));
                pos = MidRootWithMpcLog(deepTree, shallowTree, logFile, matchIdxShift + i, shallow, deep, minimum);
                if (enableLog)
                    printf("探索ノード数：%zu[Node]  推定CPUスコア：%.1f\n",
                           deepTree->nodeCount, deepTree->score / (float)(STONE_VALUE));
            }
            // 合法手判定
            assert(BoardIsLegalTT(board, pos));

            // 実際に着手
            BoardPutTT(board, pos);
            nbEmpty--;

        } //end of loop:　while (!BoardIsFinished(board))
        printf("Game %d Finished                 \n", i);
    }

    TreeDelete(deepTree);
    TreeDelete(shallowTree);
    fclose(logFile);
}

int main(int argc, char **argv)
{
    HashInit();
    srand(GLOBAL_SEED);

    int idxShift;
    int nbPlay;
    uint8 shallow, deep;
    bool showBoard;

    if (argc < 6)
    {
        printf("引数が足りません\n");
        getchar();
        return 0;
    }

    nbPlay = atoi(argv[1]);
    idxShift = atoi(argv[2]);
    shallow = atoi(argv[3]);
    deep = atoi(argv[4]);
    showBoard = atoi(argv[5]);

    //SelfPlay(6, 17, false);
    MPCSampling(nbPlay, 6, 1.0 / 60.0, showBoard, idxShift, shallow, deep, 4);
    return 0;
}