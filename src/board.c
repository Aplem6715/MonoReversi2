/**
 * @file board.c
 * @author Daichi Sato
 * @brief 盤面制御に関する関数
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 * 対戦時に使われる盤面（探索時には使われない）
 * 表示・着手・戻るなどの機能や盤面情報の取得など
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "const.h"
#include "bit_operation.h"

/**
 * @brief CUI盤面の描画を行う
 * 
 * @param black 黒石情報
 * @param white 白石情報
 * @param mobility 着手可能位置
 */
static void Draw(uint64_t black, uint64_t white, uint64_t mobility)
{
    uint64_t cursor = 0x0000000000000001;
    printf("＋ー＋ー＋ー＋ー＋ー＋ー＋ー＋ー＋ー＋\n");
    printf("｜　｜ A｜ B｜ C｜ D｜ E｜ F｜ G｜ H｜\n");
    for (int y = 0; y < 17; y++)
    {
        for (int x = 0; x < 17; x++)
        {
            if (x == 0)
            {
                if (y % 2 == 0)
                {
                    printf("＋ー");
                }
                else
                {
                    printf("｜ %d", y / 2 + 1);
                }
            }

            if (y % 2 == 0)
            {
                if (x % 2 == 0)
                {
                    printf("＋");
                }
                else
                {
                    printf("ー");
                }
            }
            else
            {
                if (x % 2 == 0)
                {
                    printf("｜");
                }
                else if (cursor & black)
                {
                    printf("○");
                }
                else if (cursor & white)
                {
                    printf("●");
                }
                else if (cursor & mobility)
                {
                    printf("・");
                }
                else
                {
                    printf("　");
                }
                if (x % 2 == 1)
                {
                    cursor <<= 1;
                }
            }
        }
        printf("\n");
    }
    printf("○:%d  ●:%d\n", CountBits(black), CountBits(white));
}

/**
 * @brief 黒石情報を取得
 * 
 * @param board 盤面情報
 * @return uint64_t 黒石の情報
 */
uint64_t BoardGetBlack(Board *board) { return board->black; }

/**
 * @brief 白石の情報を取得
 * 
 * @param board 盤面情報
 * @return uint64_t 白石の情報
 */
uint64_t BoardGetWhite(Board *board) { return board->white; }

/**
 * @brief 手番の石情報を取得
 * 
 * @param board 盤面情報
 * @return uint64_t 手番の石情報
 */
uint64_t BoardGetOwn(Board *board)
{
    if (board->turn == BLACK)
    {
        return board->black;
    }
    return board->white;
}

/**
 * @brief 手番とは逆のプレイヤーの石情報を取得
 * 
 * @param board 盤面情報
 * @return uint64_t 逆手番の石情報
 */
uint64_t BoardGetOpp(Board *board)
{
    if (board->turn == BLACK)
    {
        return board->white;
    }
    return board->black;
}

/**
 * @brief 手番の色を取得
 * 
 * @param board 盤面情報
 * @return uint8 手番の色
 */
uint8 BoardGetTurnColor(Board *board) { return board->turn; }

/**
 * @brief 盤面情報のリセット
 * 
 * @param board リセットする盤面
 */
void BoardReset(Board *board)
{
    board->black = 0x0000000810000000;
    board->white = 0x0000001008000000;
    board->turn = BLACK;
    board->nbPlayed = 0;
}

/**
 * @brief 着手処理
 * 
 * @param board 盤面情報
 * @param pos 着手位置インデックス
 * @return uint64_t 反転bit位置
 */
uint64_t BoardPutTT(Board *board, uint8 pos)
{
    uint64_t flip;
    if (board->turn == BLACK)
    {
        flip = CalcFlip64(board->black, board->white, pos);
        board->black = board->black ^ flip ^ CalcPosBit(pos);
        board->white = board->white ^ flip;
    }
    else
    {
        flip = CalcFlip64(board->white, board->black, pos);
        board->black = board->black ^ flip;
        board->white = board->white ^ flip ^ CalcPosBit(pos);
    }
    // 着手情報を保存（どっちが，どこに打ち，どこを反転させたか）
    board->history[board->nbPlayed].color = board->turn;
    board->history[board->nbPlayed].pos = pos;
    board->history[board->nbPlayed].flip = flip;
    board->nbPlayed++;
    board->turn ^= 1;
    return flip;
}

/**
 * @brief 石情報を設定
 * 
 * @param board 盤面情報
 * @param black 設定する黒石情報
 * @param white 設定する白石情報
 * @param turn 設定する手番の色
 */
void BoardSetStones(Board *board, uint64_t black, uint64_t white, uint8 turn)
{
    board->black = black;
    board->white = white;
    board->turn = turn;
    board->nbPlayed = 0;
}

/**
 * @brief 着手可能な位置の中からランダムな位置を取得
 * 
 * @param board 盤面情報
 * @return uint8 ランダム着手位置
 */
uint8 BoardGetRandomPosMoveable(Board *board)
{
    uint64_t mob = BoardGetMobility(board);
    if (mob == 0)
        return 0;
    uint64_t posBit = 0x0000000000000001;
    uint8 nbMobs = CountBits(mob);
    uint8 target = rand() % nbMobs + 1;
    int ignored = 0;
    while (1)
    {
        if ((posBit & mob) != 0)
        {
            ignored++;
            if (ignored == target)
            {
                break;
            }
        }
        posBit <<= 1;
    }
    return PosIndexFromBit(posBit);
}

/**
 * @brief 待った・もとに戻す機能
 * 
 * @param board 盤面情報
 * @return bool 戻せたらtrue, ダメならfalse
 */
bool BoardUndo(Board *board)
{
    uint64_t flip, posBit;
    uint8 hist_turn;
    if (board->nbPlayed > 0)
    {
        board->nbPlayed--;

        hist_turn = board->history[board->nbPlayed].color;
        flip = board->history[board->nbPlayed].flip;
        posBit = CalcPosBit(board->history[board->nbPlayed].pos);

        if (hist_turn == BLACK)
        {
            board->black = board->black ^ flip ^ posBit;
            board->white = board->white ^ flip;
        }
        else
        {
            board->black = board->black ^ flip;
            board->white = board->white ^ flip ^ posBit;
        }
        board->turn = hist_turn;
    }
    else
    {
        printf("これ以上戻せません\n");
        return 0;
    }
    return 1;
}

/**
 * @brief 前の手番まで戻す
 * 
 * @param board 盤面情報
 * @return bool 戻せたかどうか
 */
bool BoardUndoUntilColorChange(Board *board)
{
    uint8 oppTurn = AntiColor(board->turn);
    if (BoardUndo(board))
    {
        while (board->turn == oppTurn)
        {
            if (!BoardUndo(board))
                return;
        }
    }
    else
    {
        return false;
    }
    return true;
}

/**
 * @brief パスする
 * 
 * @param board 盤面情報
 */
void BoardSkip(Board *board)
{
    board->turn ^= 1;
}

/**
 * @brief 盤面情報をCUIに表示する
 * 
 * @param board 盤面情報
 */
void BoardDraw(Board *board)
{
    Draw(board->black, board->white, BoardGetMobility(board));
}

/**
 * @brief 指定した色の石数を取得する
 * 
 * @param board 盤面情報
 * @param color 数える石の色
 * @return int colorの石数
 */
int BoardGetStoneCount(Board *board, uint8 color)
{
    if (color == BLACK)
    {
        return CountBits(board->black);
    }
    else
    {
        return CountBits(board->white);
    }
}

/**
 * @brief 手番側の着手可能位置を取得
 * 
 * @param board 盤面情報
 * @return uint64_t 着手可能位置bit
 */
uint64_t BoardGetMobility(Board *board)
{
    return BoardGetColorsMobility(board, board->turn);
}

/**
 * @brief colorの着手可能位置を取得
 * 
 * @param board 盤面情報
 * @param color 計算対象の色
 * @return uint64_t 着手可能位置bit
 */
uint64_t BoardGetColorsMobility(Board *board, uint8 color)
{
    if (color == BLACK)
    {
        return CalcMobility64(board->black, board->white);
    }
    else
    {
        return CalcMobility64(board->white, board->black);
    }
}

/**
 * @brief 着手位置が本当に着手可能かを判定する
 * 
 * @param board 盤面情報
 * @param pos 着手位置インデックス
 * @return bool 着手可能/着手違反
 */
bool BoardIsLegalTT(Board *board, uint8 pos)
{
    return (BoardGetMobility(board) & CalcPosBit(pos)) != 0;
}

/**
 * @brief 終局判定
 * 
 * @param board 盤面情報
 * @return bool 終局でtrue
 */
bool BoardIsFinished(Board *board)
{
    if (board->nbPlayed >= HIST_LENGTH)
    {
        return true;
    }
    return (CalcMobility64(board->black, board->white) == 0) && (CalcMobility64(board->white, board->black) == 0);
}
