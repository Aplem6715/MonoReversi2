
/**
 * @file game.c
 * @author Daichi Sato
 * @brief CUIでの対戦の管理・処理を行う
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 * GameInit()を呼び出した後にGameStart()を実行することでゲームが開始される。
 * Game構造体にゲーム進行のための情報が保持される。
 * 人対人や，人対CPUの対戦が可能。
 * 
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "game.h"
#include "board.h"
#include "search/search.h"
#include "bit_operation.h"

/**
 * @brief 対戦の初期化
 * 
 * @param game 対戦情報
 * @param black 黒のプレイヤー属性（人間・AI
 * @param white 白のプレイヤー属性
 * @param mid 中盤探索深度
 * @param end 終盤探索深度
 */
void GameInit(Game *game, PlayerEnum black, PlayerEnum white, int mid, int end)
{
    game->player[WHITE] = white;
    game->player[BLACK] = black;

    // AIの初期化
    if (game->player[WHITE] == AI)
    {
        InitTree(&game->tree[WHITE], mid, end, 4, 8, 1, 1, 0, 0);
    }
    if (game->player[BLACK] == AI)
    {
        InitTree(&game->tree[BLACK], mid, end, 4, 8, 1, 1, 0, 0);
    }
    GameReset(game);
}

/**
 * @brief 対戦情報の解放
 * 
 * @param game 対戦情報
 */
void GameFree(Game *game)
{
    // AIの初期化
    if (game->player[WHITE] == AI)
    {
        DeleteTree(&game->tree[WHITE]);
    }
    if (game->player[BLACK] == AI)
    {
        DeleteTree(&game->tree[BLACK]);
    }
}

/**
 * @brief CUI入力から着手位置を取得
 * 
 * @param game 対戦情報
 * @return uint8 着手位置インデックス
 */
uint8 WaitPosHumanInput(Game *game)
{
    char str_pos[5];
    uint8 posIdx;

    while (true)
    {
        printf("位置を入力してください（A1～H8）:");
        fgets(str_pos, 5, stdin);

        posIdx = PosIndexFromAscii(str_pos);

        if (posIdx == NOMOVE_INDEX)
        {
            printf("(%c, %c) には置けません\n", str_pos[0], str_pos[1]);
        }
        else if (posIdx == UNDO_INDEX)
        {
            return UNDO_INDEX;
        }
        else
        {
            return 63 - posIdx;
        }
    }
}

/**
 * @brief AIの着手位置を取得
 * 
 * @param game 対戦情報
 * @param color 手番色
 * @return uint8 着手位置
 */
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

/**
 * @brief 着手を待機
 * 
 * 人間かAIかで処理を分ける
 * 
 * @param game 対戦情報
 * @param color 手番の色
 * @return uint8 着手位置インデックス
 */
uint8 WaitPos(Game *game, uint8 color)
{
    if (game->player[color] == HUMAN)
    {
        return WaitPosHumanInput(game);
    }
    else if (game->player[color] == AI)
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

/**
 * @brief 対戦をリセットする
 * 
 * @param game 対戦情報
 */
void GameReset(Game *game)
{
    int i;
    for (i = 0; i < 60; i++)
    {
        game->moves[i] = NOMOVE_INDEX;
    }
    BoardReset(game->board);
    game->turn = 0;
}

/**
 * @brief 対戦を行う
 * 
 * @param game 対戦情報
 */
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
        if (pos == UNDO_INDEX)
        {
            // 人間同士対戦なら一手戻す
            if (game->player[0] == game->player[1])
            {
                BoardUndo(game->board);
            }
            else //AI対戦なら直前の自分の手まで戻す
            {
                BoardUndoUntilColorChange(game->board);
            }
            printf("戻しました");
            continue;
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
            printf("%sが%c%dに置きました\n",
                   (BoardGetTurnColor(game->board) == BLACK ? "○" : "●"),
                   (char)('A' + pos % 8),
                   pos / 8 + 1);
            game->moves[game->turn] = pos;
            game->turn++;
        }

        // 実際に着手
        BoardPutTT(game->board, pos);

    } //end of loop:　while (!BoardIsFinished(game->board))

    // 勝敗を表示
    BoardDraw(game->board);
    int numBlack = BoardGetStoneCount(game->board, BLACK);
    int numWhite = BoardGetStoneCount(game->board, WHITE);

    if (numBlack == numWhite)
    {
        printf("引き分け！！\n");
    }
    else if (numBlack > numWhite)
    {
        printf("○の勝ち！\n");
    }
    else
    {
        printf("●の勝ち！\n");
    }

    char xAscii;
    int y;
    int i;
    for (i = 0; i < 60; i++)
    {
        if (game->moves[i] == NOMOVE_INDEX)
            break;
        CalcPosAscii(game->moves[i], &xAscii, &y);
        printf("%c%d, ", xAscii, y);
    }
    printf("\n");
    getchar();
    getchar();
}

/**
 * @brief 指定色側の探索木を取得
 * 
 * @param game 対戦情報
 * @param color 色指定
 * @return SearchTree* 指定色の探索木
 */
SearchTree *GetTree(Game *game, uint8 color)
{
    return &game->tree[color];
}