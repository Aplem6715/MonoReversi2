#include <stdio.h>
#include "game.h"

int main()
{
    Game game = Game(PlayerEnum::AI, PlayerEnum::AI);
    game.Start();

    return 1;
}