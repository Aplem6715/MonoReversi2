#include <iostream>
#include <string>
#include <stdio.h>
#include <chrono>
#include <thread>

#include "game.h"
//#include "ai/mcts.h"
#include "search/search.h"
#include "bit_operation.h"

Game::Game(GameMode mode)
{
    this->mode = mode;
}

Game::~Game() {}

uint8 SelectColor()
{
    int color;
    std::cout << "あなたの色を入力してください（黒："
              << (int)Const::BLACK
              << ", 白:"
              << (int)Const::WHITE
              << "）:";
    std::cin >> color;
    return color;
}

uint64 WaitPosInput()
{
    std::string str_pos;
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
        y = atoi(&str_pos[1]) - 1;

        if (x >= 0 && x < 8 && y >= 0 && y < 8)
        {
            return 0x8000000000000000 >> (x + y * 8);
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
    //MCTS *ai;
    SearchTree tree;
    if (mode != GameMode::HUMAN_VS_HUMAN)
    {
        //ai = new MCTS(1);
    }

    if (mode == GameMode::HUMAN_VS_CPU)
    {
        InitTree(&tree, 8);
        humanColor = SelectColor();
    }

    board.Reset();
    while (!board.IsFinished())
    {
        board.Draw();
        if (board.GetMobility() == 0)
        {
            std::cout << (board.GetTurnColor() == Const::BLACK ? "○の" : "●の")
                      << "置く場所がありません！スキップ！\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            board.Skip();
            continue;
        }
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
            printf("※考え中・・・\r");
            input = Search(&tree, board.GetOwn(), board.GetOpp());
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
        else
        {
            std::cout
                << (board.GetTurnColor() == Const::BLACK ? "○が" : "●が")
                << (char)('A' + idx % 8)
                << idx / 8 + 1
                << "に置きました\n";
        }
        board.Put(input);
    }
    board.Draw();
    int numBlack = board.GetStoneCount(Const::BLACK);
    int numWhite = board.GetStoneCount(Const::WHITE);
    std::cout << ((numBlack == numWhite)
                      ? "引き分け！！"
                      : ((numBlack > numWhite) ? "○の勝ち！" : "●の勝ち！"));
    getchar();
    getchar();
}