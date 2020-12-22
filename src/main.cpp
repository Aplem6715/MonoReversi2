#include <stdio.h>
#include "game.h"

int main()
{
    Game game = Game(PlayerEnum::HUMAN, PlayerEnum::HUMAN);
    game.Start();

    return 1;
}