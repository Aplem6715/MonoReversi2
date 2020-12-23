#include <stdio.h>
#include <time.h>
#include "game.h"
#include "search/bench.h"

int main()
{
    srand((unsigned int)time(NULL));
    //Game game = Game(PlayerEnum::HUMAN, PlayerEnum::AI);
    //game.Start();

    BenchSearching();

    return 1;
}