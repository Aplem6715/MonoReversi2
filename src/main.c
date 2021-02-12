#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "bit_operation.h"

//#include "search/bench.hpp"
#include "game.h"

int main()
{
    srand((unsigned int)time(NULL));
    HashInit();
    Game game[1];
    GameInit(game, HUMAN, AI, 15, 20);
    //GameInit(game, AI, HUMAN, 15, 20);
    GameStart(game);

    //std::vector<unsigned char> depths = {10, 12, 14};
    //std::vector<unsigned char> depths = {8, 10, 12};
    //char x;
    //int y;
    //CalcPosAscii(C7, x, y);
    //BenchSearching(depths, 1, 4, 8, "./resources/bench/search.txt");
    //MakeBench(2, 38);

    return 1;
}