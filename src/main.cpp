#include <stdio.h>
#include "game.h"
#include "ai/model.h"

int main()
{
    ACModel model(net_phase::test);

    Game game = Game(GameMode::HUMAN_VS_CPU);
    game.Start();

    return 1;
}