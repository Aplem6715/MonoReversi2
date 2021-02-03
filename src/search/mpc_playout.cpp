
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
    SearchTree tree[1];
    Board board;
    uint8 pos;
    uint8 nbEmpty;
    FILE *logFile;
    int i;

    InitTree(tree, 0, 8, 4, 8, 1);
    logFile = fopen(MPC_RAW_FILE, "a");
    fprintf(logFile, "matchIdx,nbEmpty,depth,score\n");

    for (i = 0; i < nbPlay; i++)
    {
        nbEmpty = 60;
        board.Reset();
        while (!board.IsFinished())
        {
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
            if (nbEmpty > tree->endDepth && ((nbEmpty >= 60 - randomTurns) || rnd_prob(mt) < randMoveRatio))
            {
                // ランダム着手位置
                pos = board.GetRandomPosMoveable();
                //printf("Random!!!\n");
            }
            else
            {
                SearchSetup(tree, board.GetOwn(), board.GetOpp());
                pos = MidRootWithMpcLog(tree, logFile, matchIdxShift + i, shallow, deep, minimum);
                if (nbEmpty <= tree->endDepth)
                {
                    printf("EndSearching            \r");
                    pos = Search(tree, board.GetOwn(), board.GetOpp(), 0);
                }
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
        printf("Game %d Finished\n", i);
    }

    fclose(logFile);
}

int main(int argc, char **argv)
{
    HashInit();
    srand((unsigned int)time(NULL));

    int idxShift;
    int nbPlay;
    uint8 shallow, deep;

    nbPlay = atoi(argv[1]);
    idxShift = atoi(argv[2]);
    shallow = atoi(argv[3]);
    deep = atoi(argv[4]);

    //SelfPlay(6, 17, false);
    MPCSampling(nbPlay, 6, 1.0 / 60.0, 1, idxShift, shallow, deep, 8);
}