#include <fstream>
#include <string>

#include <assert.h>
#include <stdio.h>

#include "game.h"
#include "bit_operation.h"

using namespace std;

void LoadGameRecords(const char *file, vector<vector<uint8>> &records)
{
    ifstream infile(file);
    string str;

    if (infile.fail())
    {
        fprintf(stderr, "Failed to open file.");
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

void GameInit(Game *game, PlayerEnum black, PlayerEnum white, int mid, int end)
{
    game->player[WHITE] = white;
    game->player[BLACK] = black;

    // AIの初期化
    if (game->player[WHITE] == PlayerEnum::AI)
    {
        InitTree(&game->tree[WHITE], mid, end, 4, 8, 1, 1, 1);
    }
    if (game->player[BLACK] == PlayerEnum::AI)
    {
        InitTree(&game->tree[BLACK], mid, end, 4, 8, 1, 1, 1);
    }
    GameReset(game);
}

void GameFree(Game *game)
{
    // AIの初期化
    if (game->player[WHITE] == PlayerEnum::AI)
    {
        DeleteTree(&game->tree[WHITE]);
    }
    if (game->player[BLACK] == PlayerEnum::AI)
    {
        DeleteTree(&game->tree[BLACK]);
    }
}

uint8 WaitPosHumanInput(Game *game)
{
    char str_pos[5];
    int x, y;

    while (true)
    {
        printf("位置を入力してください（A1～H8）:");
        fgets(str_pos, 5, stdin);
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
            printf("(%d, %d) には置けません\n", x, y);
        }
    }
}

uint8 WaitPosAI(Game *game, uint8 color)
{
    uint8 input;
    printf("※考え中・・・\r");
    input = Search(&game->tree[color], BoardGetOwn(game->board), BoardGetOpp(game->board), 0);
    printf("思考時間：%.2f[s]  探索ノード数：%zu[Node]  探索速度：%.1f[Node/s]  推定CPUスコア：%.1f",
           game->tree[color].usedTime, game->tree[color].nodeCount, game->tree[color].nodeCount / game->tree[color].usedTime, game->tree[color].score / (float)(STONE_VALUE));
    if (game->tree[color].isEndSearch)
    {
        printf("(WLD)");
    }
    printf("\n");
    return input;
}

uint8 WaitPos(Game *game, uint8 color)
{
    if (game->player[color] == PlayerEnum::HUMAN)
    {
        return WaitPosHumanInput(game);
    }
    else if (game->player[color] == PlayerEnum::AI)
    {
        return WaitPosAI(game, color); // TODO
    }
    else
    {
        printf("Unreachable");
        assert(0);
        return 0;
    }
}

void GameReset(Game *game)
{
    game->moves.clear();
    BoardReset(game->board);
    game->turn = 0;
}

void GameStart(Game *game)
{
    uint8 pos;
    game->turn = 0;
    GameReset(game);

    while (!BoardIsFinished(game->board))
    {
        BoardDraw(game->board);
        if (BoardGetMobility(game->board) == 0)
        {
            if (BoardGetTurnColor(game->board) == BLACK)
                printf("○の");
            else
                printf("●の");
            printf("置く場所がありません！スキップ！(確認できたらEnterを！)\n");
            getchar();
            BoardSkip(game->board);
            continue;
        }

        // 入力/解析の着手位置を待機
        pos = WaitPos(game, BoardGetTurnColor(game->board));
        if (pos == UNDO)
        {
            BoardUndoUntilColorChange(game->board);
            BoardUndoUntilColorChange(game->board);
        }

        // 合法手判定
        if (!BoardIsLegalTT(game->board, pos))
        {
            printf("%c%dには置けません\n",
                   (char)('A' + pos % 8),
                   pos / 8 + 1);
            continue;
        }
        else
        {
            printf("%cが%c%dに置きました\n",
                   (BoardGetTurnColor(game->board) == BLACK ? '○' : '●'),
                   (char)('A' + pos % 8),
                   pos / 8 + 1);
            game->moves.push_back(pos);
            game->turn++;
        }

        // 実際に着手
        BoardPutTT(game->board, pos);

    } //end of loop:　while (!BoardIsFinished(game->board))

    // 勝敗を表示
    BoardDraw(game->board);
    int numBlack = BoardGetStoneCount(game->board, BLACK);
    int numWhite = BoardGetStoneCount(game->board, WHITE);
    printf((
        (numBlack == numWhite) ? "引き分け！！"
                               : ((numBlack > numWhite) ? "○の勝ち！" : "●の勝ち！\n")));
    char xAscii;
    int y;
    for (uint8 move : game->moves)
    {
        CalcPosAscii(move, xAscii, y);
        printf("%c%d, ", xAscii, y);
    }
    printf("\n");
    getchar();
    getchar();
}

SearchTree *GetTree(Game *game, uint8 color)
{
    return &game->tree[color];
}