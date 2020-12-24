#include <fstream>
#include <iostream>
#include <string>
#include <stdio.h>
#include <chrono>
#include <thread>

#include "game.h"
//#include "ai/mcts.h"
#include "bit_operation.h"

using namespace std;

void LoadGameRecords(char *file, vector<vector<uint8>> &records)
{
    ifstream infile(file);
    string str;

    if (infile.fail())
    {
        cerr << "Failed to open file." << endl;
        return;
    }

    vector<uint8> moves;
    while (getline(infile, str))
    {
        moves.clear();
        for (size_t i = 0; i < str.length(); i += 2)
        {
            moves.push_back(CalcPosIndex(str.substr(i, 2).c_str()));
        }
        records.push_back(moves);
    }
}

Game::Game(PlayerEnum white, PlayerEnum black)
{
    this->player[Const::WHITE] = white;
    this->player[Const::BLACK] = black;

    // AIの初期化
    if (player[Const::WHITE] == PlayerEnum::AI)
    {
        InitTree(&tree[Const::WHITE], 5);
    }
    if (player[Const::BLACK] == PlayerEnum::AI)
    {
        InitTree(&tree[Const::BLACK], 5);
    }
    Reset();
}

Game::~Game()
{
    // AIの初期化
    if (player[Const::WHITE] == PlayerEnum::AI)
    {
        DeleteTree(&tree[Const::WHITE]);
    }
    if (player[Const::BLACK] == PlayerEnum::AI)
    {
        DeleteTree(&tree[Const::BLACK]);
    }
}

uint64 WaitPosHumanInput()
{
    string str_pos;
    int x, y;

    while (true)
    {
        cout << "位置を入力してください（A1～H8）:";
        cin >> str_pos;
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
            cout << x << "," << y << " には置けません\n";
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

void Game::Reset()
{
    moves.clear();
    board.Reset();
    turn = 0;
}

void Game::Start()
{
    uint64 input;
    turn = 0;
    Reset();

    while (!board.IsFinished())
    {
        board.Draw();
        if (board.GetMobility() == 0)
        {
            cout << (board.GetTurnColor() == Const::BLACK ? "○の" : "●の")
                 << "置く場所がありません！スキップ！\n";
            this_thread::sleep_for(chrono::seconds(1));
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
        uint8 posIdx = CalcPosIndex(input);
        // 合法手判定
        if (!board.IsLegal(input))
        {
            cout
                << (char)('A' + posIdx % 8)
                << posIdx / 8 + 1
                << "には置けません\n";
            continue;
        }
        else
        {
            cout
                << (board.GetTurnColor() == Const::BLACK ? "○が" : "●が")
                << (char)('A' + posIdx % 8)
                << posIdx / 8 + 1
                << "に置きました\n";
            moves.push_back(posIdx);
            turn++;
        }

        // 実際に着手
        board.Put(input);

    } //end of loop:　while (!board.IsFinished())

    // 勝敗を表示
    board.Draw();
    int numBlack = board.GetStoneCount(Const::BLACK);
    int numWhite = board.GetStoneCount(Const::WHITE);
    cout << ((numBlack == numWhite)
                 ? "引き分け！！"
                 : ((numBlack > numWhite) ? "○の勝ち！" : "●の勝ち！\n"));
    char xAscii;
    int y;
    for (uint8 move : moves)
    {
        CalcPosAscii(move, xAscii, y);
        printf("%c%d, ", xAscii, y);
    }
    printf("\n");
    getchar();
    getchar();
}