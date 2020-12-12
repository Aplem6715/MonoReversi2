#include <iostream>
#include <string>
#include <stdio.h>
#include "game.h"
#include "bit_operation.h"

Game::Game(GameMode mode)
{
    this->mode = mode;
}

Game::~Game() {}

uint8 SelectColor()
{
    uint8 color;
    std::cout << "あなたの色を入力してください（黒："
              << Const::BLACK
              << ", 白:"
              << Const::WHITE
              << "）:";
    std::cin >> color;
    return color;
}

uint64 WaitPosInput()
{
    std::string str_pos;
    uint64 pos;
    int x, y;

    while (true)
    {
        std::cout << "位置を入力してください（A1～H8）:";
        std::cin >> str_pos;
        if (str_pos[0] == 'U')
        {
            return Const::UNDO;
        }

        x = str_pos[0] - 'A';
        y = atoi(&str_pos[1]);

        if (x >= 0 && x < 8 && y >= 0 && y < 8)
        {
            return 0x8000000000000000 >> (x + (y - 1) * 8);
        }
        else
        {
            std::cout << x << "," << y << " には置けません\n";
        }
    }
}

void Game::Start()
{
    uint8 humanColor;
    uint64 input;
    if (mode == GameMode::HUMAN_VS_CPU)
    {
        humanColor = SelectColor();
    }

    board.Reset();
    while (!board.IsFinished())
    {
        board.Draw();
        if (mode == GameMode::HUMAN_VS_HUMAN || board.GetTurnColor() == humanColor)
        {
            input = WaitPosInput();
            if (input == Const::UNDO)
            {
                do
                {
                    board.Undo();
                } while (mode != GameMode::HUMAN_VS_HUMAN && board.GetTurnColor() != humanColor);
                continue;
            }
        }
        else
        {
            input = 0; //AI 未実装
        }

        int idx = CalcPosIndex(input);
        if (!board.IsLegal(input))
        {
            std::cout
                << (char)('A' + idx % 8)
                << idx / 8 + 1
                << "には置けません\n";
            continue;
        }
        board.Put(input);
    }
}