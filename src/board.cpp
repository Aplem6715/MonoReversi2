#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "bit_operation.h"

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

uint64_t BoardGetBlack(Board *board) { return board->black; }
uint64_t BoardGetWhite(Board *board) { return board->white; }

uint64_t BoardGetOwn(Board *board)
{
    if (board->turn == BLACK)
    {
        return board->black;
    }
    return board->white;
}

uint64_t BoardGetOpp(Board *board)
{
    if (board->turn == BLACK)
    {
        return board->white;
    }
    return board->black;
}

uint8 BoardGetTurnColor(Board *board) { return board->turn; }

void BoardReset(Board *board)
{
    board->black = 0x0000000810000000;
    board->white = 0x0000001008000000;
    board->turn = BLACK;
    board->nbPlayed = 0;
}

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

void BoardSetStones(Board *board, uint64_t black, uint64_t white, uint8 turn)
{
    board->black = black;
    board->white = white;
    board->turn = turn;
}

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
    return CalcPosIndex(posBit);
}

int BoardUndo(Board *board)
{
    uint64_t flip, pos;
    uint8 hist_turn;
    if (board->nbPlayed > 0)
    {
        board->nbPlayed--;
        hist_turn = board->history[board->nbPlayed].color;
        flip = board->history[board->nbPlayed].flip;
        pos = board->history[board->nbPlayed].pos;
        if (hist_turn == BLACK)
        {
            board->black = board->black ^ flip ^ pos;
            board->white = board->white ^ flip;
        }
        else
        {
            board->black = board->black ^ flip;
            board->white = board->white ^ flip ^ pos;
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

void BoardUndoUntilColorChange(Board *board)
{
    uint8 prev_turn = board->turn;
    while (prev_turn == board->turn)
    {
        BoardUndo(board);
    }
}

void BoardSkip(Board *board)
{
    board->turn ^= 1;
}

void BoardDraw(Board *board)
{
    Draw(board->black, board->white, BoardGetMobility(board));
}

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

uint64_t BoardGetMobility(Board *board)
{
    return BoardGetMobility(board, board->turn);
}

uint64_t BoardGetMobility(Board *board, uint8 color)
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

bool BoardIsLegalTT(Board *board, uint8 pos)
{
    return (BoardGetMobility(board) & CalcPosBit(pos)) != 0;
}

bool BoardIsFinished(Board *board)
{
    if (board->nbPlayed >= HIST_LENGTH)
    {
        return true;
    }
    return (CalcMobility64(board->black, board->white) == 0) && (CalcMobility64(board->white, board->black) == 0);
}
