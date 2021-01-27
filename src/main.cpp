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
    //Game game = Game(PlayerEnum::HUMAN, PlayerEnum::AI, 10, 21);
    //Game game = Game(PlayerEnum::AI, PlayerEnum::HUMAN, 10, 21);
    //game.Start();

    std::vector<unsigned char> depths = {8, 10, 12};
    //char x;
    //int y;
    //CalcPosAscii(C7, x, y);
    BenchSearching(depths, 1, 3, 4, 4);
    //MakeBench(2, 50);

    return 1;
}