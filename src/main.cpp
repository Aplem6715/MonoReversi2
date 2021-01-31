#include <stdio.h>
#include <time.h>
#include <vector>
#include "game.h"
#include "search/bench.h"
#include "bit_operation.h"

int main()
{
    srand((unsigned int)time(NULL));
    HashInit();
    //Game game = Game(PlayerEnum::HUMAN, PlayerEnum::AI, 12, 22);
    Game game = Game(PlayerEnum::AI, PlayerEnum::HUMAN, 12, 22);
    game.Start();

    //std::vector<unsigned char> depths = {10, 12, 14};
    //std::vector<unsigned char> depths = {8, 10, 12};
    //char x;
    //int y;
    //CalcPosAscii(C7, x, y);
    //BenchSearching(depths, 1, 4, 8, "./resources/bench/search.txt");
    //MakeBench(2, 38);

    return 1;
}