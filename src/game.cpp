#include <fstream>
#include <iostream>
#include <string>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <assert.h>

#include "game.h"
//#include "ai/mcts.h"
#include "bit_operation.h"

using namespace std;

void LoadGameRecords(const char *file, vector<vector<uint8>> &records)
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

Game::Game(PlayerEnum black, PlayerEnum white, int mid, int end)
{
    this->player[WHITE] = white;
    this->player[BLACK] = black;

    // AIの初期化
    if (player[WHITE] == PlayerEnum::AI)
    {
        InitTree(&tree[WHITE], mid, end, 4, 8, 1, 1, 1);
    }
    if (player[BLACK] == PlayerEnum::AI)
    {
        InitTree(&tree[BLACK], mid, end, 4, 8, 1, 1, 1);
    }
    Reset();
}

Game::~Game()
{
    // AIの初期化
    if (player[WHITE] == PlayerEnum::AI)
    {
        DeleteTree(&tree[WHITE]);
    }
    if (player[BLACK] == PlayerEnum::AI)
    {
        DeleteTree(&tree[BLACK]);
    }
}

uint8 WaitPosHumanInput()
{
    string str_pos;
    int x, y;

    while (true)
    {
        cout << "位置を入力してください（A1～H8）:";
        cin >> str_pos;
        if (str_pos[0] == 'U')
        {
            return UNDO;
        }

        x = str_pos[0] - 'A';
        y = atoi(&str_pos[1]) - 1;

        if (x >= 0 && x < 8 && y >= 0 && y < 8)
        {
            return x + y * 8;
        }
        else
        {
            cout << x << "," << y << " には置けません\n";
        }
    }
}

uint8 Game::WaitPosAI(uint8 color)
{
    uint8 input;
    printf("※考え中・・・\r");
    input = Search(&tree[color], BoardGetOwn(board), BoardGetOpp(board), 0);
    printf("思考時間：%.2f[s]  探索ノード数：%zu[Node]  探索速度：%.1f[Node/s]  推定CPUスコア：%.1f",
           tree[color].usedTime, tree[color].nodeCount, tree[color].nodeCount / tree[color].usedTime, tree[color].score / (float)(STONE_VALUE));
    if (tree[color].isEndSearch)
    {
        printf("(WLD)");
    }
    printf("\n");
    return input;
}

uint8 Game::WaitPos(uint8 color)
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
    BoardReset(board);
    turn = 0;
}

void Game::Start()
{
    uint8 pos;
    turn = 0;
    Reset();

    while (!BoardIsFinished(board))
    {
        BoardDraw(board);
        if (BoardGetMobility(board) == 0)
        {
            cout << (BoardGetTurnColor(board) == BLACK ? "○の" : "●の")
                 << "置く場所がありません！スキップ！\n";
            this_thread::sleep_for(chrono::seconds(1));
            BoardSkip(board);
            continue;
        }

        // 入力/解析の着手位置を待機
        pos = WaitPos(BoardGetTurnColor(board));
        if (pos == UNDO)
        {
            BoardUndoUntilColorChange(board);
            BoardUndoUntilColorChange(board);
        }

        // 合法手判定
        if (!BoardIsLegalTT(board, pos))
        {
            cout
                << (char)('A' + pos % 8)
                << pos / 8 + 1
                << "には置けません\n";
            continue;
        }
        else
        {
            cout
                << (BoardGetTurnColor(board) == BLACK ? "○が" : "●が")
                << (char)('A' + pos % 8)
                << pos / 8 + 1
                << "に置きました\n";
            moves.push_back(pos);
            turn++;
        }

        // 実際に着手
        BoardPutTT(board, pos);

    } //end of loop:　while (!BoardIsFinished(board))

    // 勝敗を表示
    BoardDraw(board);
    int numBlack = BoardGetStoneCount(board, BLACK);
    int numWhite = BoardGetStoneCount(board, WHITE);
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

SearchTree *Game::GetTree(uint8 color)
{
    return &tree[color];
}