#include <stdio.h>
#include "game.h"
#include "search/bench.h"

int main()
{
    //Game game = Game(PlayerEnum::HUMAN, PlayerEnum::AI);
    //game.Start();

    MakeBench(10000, 16);

    return 1;
}