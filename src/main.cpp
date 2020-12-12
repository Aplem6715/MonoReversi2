#include <stdio.h>
#include "game.h"

int main()
{
    Game game = Game(GameMode::HUMAN_VS_HUMAN);
    game.Start();
    return 1;
}