
#include <fstream>
#include <iostream>

#include "bench.h"
#include "../board.h"
#include "../game.h"
#include "../bit_operation.h"

using namespace std;

void MakeBench(int nbGames, uint8 nbRandomTurn)
{
    Board board;
    uint64 pos;
    uint8 turn;
    char xAscii;
    int y;
    uint8 moves[60];

    ofstream outfile(SEARCH_BENCH_FILE, ios::app);

    for (int i = 0; i < nbGames; i++)
    {
        board.Reset();
        for (turn = 0; turn < nbRandomTurn; turn++)
        {
            pos = board.MoveRandom();
            if (pos == 0)
            {
                break;
            }
            moves[turn] = CalcPosIndex(pos);
        }

        if (pos == 0)
        {
            i--;
            continue;
        }

        for (turn = 0; turn < nbRandomTurn; turn++)
        {
            CalcPosAscii(moves[turn], xAscii, y);
            outfile << xAscii << y;
        }

        outfile << "\n";
    }
}

void Bench1Game(SearchTree &tree, vector<uint8> moves, int nbPut, ofstream &logfile)
{
    Board board;
    uint64 pos;

    board.Reset();
    for (uint8 move : moves)
    {
        char x;
        int y;
        CalcPosAscii(move, x, y);
        logfile << x << y;
        board.Put(CalcPosBit(move));
    }
    logfile << "\n";

    board.Draw();
    for (int i = 0; i < nbPut; i++)
    {
        if (board.IsFinished())
        {
            break;
        }
        else if (board.GetMobility() == 0)
        {
            board.Skip();
        }
        pos = Search(&tree, board.GetOwn(), board.GetOpp());
        board.Put(pos);
        board.Draw();
        logfile << tree.depth << ","
                << tree.usedTime << ","
                << tree.nodeCount << ","
                << tree.nodeCount / tree.usedTime << ","
                << tree.score << "\n";
    }
}

void BenchSearching()
{
    SearchTree tree;
    vector<vector<uint8>> records;
    ofstream logfile(BENCH_LOG_FILE);
    unsigned char depth = 4;

    std::chrono::system_clock::time_point start, end;

    logfile.setf(ios::fixed, ios::floatfield);
    logfile.precision(2);
    logfile << "探索深度：\t  思考時間：\t  探索ノード数：\t  探索速度：\t  推定CPUスコア：\t\n";
    LoadGameRecords(SEARCH_BENCH_FILE, records);

    for (vector<uint8> moves : records)
    {
        depth = 4;
        do
        {
            InitTree(&tree, depth);

            start = std::chrono::system_clock::now();
            Bench1Game(tree, moves, 2, logfile);
            end = std::chrono::system_clock::now();

            depth += 1;
        } while (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() < 3 * 2 * 1000 && depth < 20);
    }

    logfile.unsetf(ios::floatfield);
    logfile.close();
}
