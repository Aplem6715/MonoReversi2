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

    std::vector<unsigned char> depths = {22};
    //char x;
    //int y;
    //CalcPosAscii(C7, x, y);
    BenchSearching(depths, 1, 7, 8, 8);
    //MakeBench(2, 38);

    return 1;
}