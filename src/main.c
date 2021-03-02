/**
 * @file main.c
 * @author Daichi Sato
 * @brief CUIプレイ時のメイン関数
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "bit_operation.h"

#include "game.h"

int main()
{
    srand(GLOBAL_SEED);
    HashInit();
    Game game[1];

    // 現状ソースコードで先行/後攻切り替え
    //GameInit(game, GM_CPU_WHITE, 12, 20);
    GameInit(game, GM_CPU_BLACK, 12, 20);
    GameStart(game);

    // ベンチマークの名残
    //std::vector<unsigned char> depths = {10, 12, 14};
    //std::vector<unsigned char> depths = {8, 10, 12};
    //char x;
    //int y;
    //CalcPosAscii(C7, x, y);
    //BenchSearching(depths, 1, 4, 8, "./resources/bench/search.txt");
    //MakeBench(2, 38);

    return 0;
}