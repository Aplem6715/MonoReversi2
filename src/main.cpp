#include <stdio.h>
#include <time.h>
#include <vector>
#include "game.h"
#include "search/bench.h"

int main()
{
    srand((unsigned int)time(NULL));
    InitHash();
    Game game = Game(PlayerEnum::AI, PlayerEnum::HUMAN);
    game.Start();

    //std::vector<unsigned char> depths = {4, 6, 8, 10};

    //BenchSearching(depths, 1, 2, 100);

    return 1;
}