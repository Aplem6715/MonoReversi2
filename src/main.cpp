#include <stdio.h>
#include <time.h>
#include "game.h"
#include "search/bench.h"

int main()
{
    srand((unsigned int)time(NULL));
    //Game game = Game(PlayerEnum::HUMAN, PlayerEnum::AI);
    //game.Start();

    MakeBench(5, 6);
    MakeBench(5, 16);
    MakeBench(5, 26);
    MakeBench(5, 36);
    MakeBench(5, 46);

    return 1;
}