#include <iostream>
#include <string>
#include <stdio.h>
#include <chrono>
#include <thread>

#include "game.h"
//#include "ai/mcts.h"
#include "bit_operation.h"

Game::Game(PlayerEnum white, PlayerEnum black)
{
    this->player[Const::WHITE] = white;
    this->player[Const::BLACK] = black;
}

Game::~Game() {}

uint64 WaitPosHumanInput()
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

uint64 Game::WaitPosAI(uint8 color)
{
    uint64 input;
    printf("※考え中・・・\r");
    input = Search(&tree[color], board.GetOwn(), board.GetOpp());
    printf("思考時間：%.2f[s]  探索ノード数：%zu[Node]  探索速度：%.1f[Node/s]  推定CPUスコア：%.1f\n",
           tree[color].usedTime, tree[color].nodeCount, tree[color].nodeCount / tree[color].usedTime, tree[color].score);
    return input;
}

uint64 Game::WaitPos(uint8 color)
{
    if (player[color] == PlayerEnum::HUMAN)
    {
        return WaitPosHumanInput();
    }
    else if (player[color] == PlayerEnum::AI)
    {
        return WaitPosAI(color); // TODO
    }
    else
    {
        printf("Unreachable");
        assert(0);
        return 0;
    }
}

void Game::Start()
{
    uint64 input;

    // AIの初期化
    if (player[Const::WHITE] == PlayerEnum::AI)
    {
        InitTree(&tree[Const::WHITE], 8);
    }
    if (player[Const::BLACK] == PlayerEnum::AI)
    {
        InitTree(&tree[Const::BLACK], 8);
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

        // 入力/解析の着手位置を待機
        input = WaitPos(board.GetTurnColor());
        if (input == Const::UNDO)
        {
            board.UndoUntilColorChange();
        }

        // 入力位置のインデックスを取得
        int idx = CalcPosIndex(input);
        // 合法手判定
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

        // 実際に着手
        board.Put(input);

    } //end of loop:　while (!board.IsFinished())

    // 勝敗を表示
    board.Draw();
    int numBlack = board.GetStoneCount(Const::BLACK);
    int numWhite = board.GetStoneCount(Const::WHITE);
    std::cout << ((numBlack == numWhite)
                      ? "引き分け！！"
                      : ((numBlack > numWhite) ? "○の勝ち！" : "●の勝ち！"));
    getchar();
    getchar();
}