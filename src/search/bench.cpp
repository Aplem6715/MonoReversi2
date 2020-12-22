
#include <fstream>

#include "bench.h"
#include "../board.h"
#include "../bit_operation.h"

void MakeBench(int nbGames, uint8 nbRandomTurn)
{
    Board board;
    uint64 pos;
    uint8 turn;
    char xAscii;
    int y;
    uint8 moves[60];

    std::ofstream outfile(SEARCH_BENCH_FILE, std::ios::app);

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

void Bench1Game(uint8 *moves){
    
}

void BenchSearching();
