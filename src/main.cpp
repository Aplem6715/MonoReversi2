#include <stdio.h>
#include <time.h>
#include <vector>
#include "game.h"
#include "search/bench.h"
#include "bit_operation.h"

int main()
{
    srand((unsigned int)time(NULL));
    InitHash();
    //Game game = Game(PlayerEnum::HUMAN, PlayerEnum::AI, 8, 10);
    //Game game = Game(PlayerEnum::AI, PlayerEnum::HUMAN, 8, 10);
    //game.Start();

    std::vector<unsigned char> depths = {8, 10, 12};
    //char x;
    //int y;
    //CalcPosAscii(C7, x, y);
    BenchSearching(depths, 1, 4, 6);
    //MakeBench(10, 10);

    return 1;
}