
#define _CRT_SECURE_NO_WARNINGS

#include "../board.h"
#include "../search/search.h"
#include "../game.h"
#include "../search/mid.h"
#include "../search/mpc.h"
#include <string>
#include <assert.h>
#include <random>
#include <fstream>
#include <iostream>
#include <time.h>

using namespace std;
static random_device rnd;
static mt19937 mt(rnd());
static uniform_real_distribution<double> rnd_prob(0.0, 1.0);

void MPCSampling(int nbPlay, int randomTurns, double randMoveRatio, uint8 enableLog, int matchIdxShift, uint8 shallow, uint8 deep, uint8 minimum)
{
    SearchTree deepTree[1], shallowTree[1];
    Board board;
    uint8 pos;
    uint8 nbEmpty;
    FILE *logFile;
    int i;

    minimum = deep > minimum ? deep : minimum;
    InitTree(shallowTree, 0, minimum, 4, 8, 1, 1);
    InitTree(deepTree, 0, minimum, 4, 8, 1, 1);
    logFile = fopen(MPC_RAW_FILE, "a");

    for (i = 0; i < nbPlay; i++)
    {
        nbEmpty = 60;
        board.Reset();
        while (!board.IsFinished())
        {
            if (nbEmpty <= deep)
            {
                break;
            }
            if (enableLog)
            {
                board.Draw();
                //_sleep(500);
            }

            // 置ける場所がなかったらスキップ
            if (board.GetMobility() == 0)
            {
                board.Skip();
                continue;
            }

            // 着手
            if (nbEmpty > deepTree->endDepth && ((nbEmpty >= 60 - randomTurns) || rnd_prob(mt) < randMoveRatio))
            {
                // ランダム着手位置
                pos = board.GetRandomPosMoveable();
                //printf("Random!!!\n");
            }
            else
            {
                SearchSetup(deepTree, board.GetOwn(), board.GetOpp());
                SearchSetup(shallowTree, board.GetOwn(), board.GetOpp());
                pos = MidRootWithMpcLog(tree, logFile, matchIdxShift + i, shallow, deep, minimum);
                if (enableLog)
                    printf("探索ノード数：%zu[Node]  推定CPUスコア：%.1f\n",
                           tree->nodeCount, tree->score / (float)(STONE_VALUE));
            }
            // 合法手判定
            assert(board.IsLegalTT(pos));

            // 実際に着手
            board.PutTT(pos);
            nbEmpty--;

        } //end of loop:　while (!board.IsFinished())
        printf("Game %d Finished                 \n", i);
    }

    DeleteTree(deepTree);
    DeleteTree(shallowTree);
    fclose(logFile);
}

int main(int argc, char **argv)
{
    HashInit();
    srand((unsigned int)time(NULL));

    int idxShift;
    int nbPlay;
    uint8 shallow, deep;
    uint8 showBoard;

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