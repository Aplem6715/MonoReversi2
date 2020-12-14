#include <stdio.h>
#include "board.h"
#include "bit_operation.h"

Board::Board(/* args */){};
Board::~Board(){};

uint64 Board::GetBlack() { return black; }
uint64 Board::GetWhite() { return white; }

uint64 Board::GetOwn()
{
    if (turn == Const::BLACK)
    {
        return black;
    }
    return white;
}

uint64 Board::GetOpp()
{
    if (turn == Const::BLACK)
    {
        return white;
    }
    return black;
}

uint8 Board::GetTurnColor() { return turn; }

void Board::Reset()
{
    black = 0x0000001008000000;
    white = 0x0000000810000000;
    turn = Const::BLACK;
    nbPlayed = 0;
}

void Board::Put(uint64 pos)
{
    uint64 flip;
    if (turn == Const::BLACK)
    {
        flip = CalcFlip(black, white, pos);
        black = black ^ flip ^ pos;
        white = white ^ flip;
    }
    else
    {
        flip = CalcFlip(white, black, pos);
        black = black ^ flip;
        white = white ^ flip ^ pos;
    }
    // 着手情報を保存（どっちが，どこに打ち，どこを反転させたか）
    history[nbPlayed].color = turn;
    history[nbPlayed].pos = pos;
    history[nbPlayed].flip = flip;
    nbPlayed++;
    turn ^= 1;
}

void Board::Undo()
{
    uint64 flip, pos;
    uint8 hist_turn;
    if (nbPlayed > 0)
    {
        nbPlayed--;
        hist_turn = history[nbPlayed].color;
        flip = history[nbPlayed].flip;
        pos = history[nbPlayed].pos;
        if (hist_turn == Const::BLACK)
        {
            black = black ^ flip ^ pos;
            white = white ^ flip;
        }
        else
        {
            black = black ^ flip;
            white = white ^ flip ^ pos;
        }
        this->turn = hist_turn;
    }
    else
    {
        printf("これ以上戻せません\n");
    }
}

void Board::Skip()
{
    turn ^= 1;
}

void Board::Draw()
{
    uint64 cursor = 0x8000000000000000;
    uint64 mobility = GetMobility();
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
                    cursor >>= 1;
                }
            }
        }
        printf("\n");
    }
    printf("○:%d  ●:%d\n", GetStoneCount(Const::BLACK), GetStoneCount(Const::WHITE));
}

int Board::GetStoneCount(uint8 color)
{
    if (color == Const::BLACK)
    {
        return CountBits(black);
    }
    else
    {
        return CountBits(white);
    }
}

uint64 Board::GetMobility()
{
    if (turn == Const::BLACK)
    {
        return CalcMobility(black, white);
    }
    else
    {
        return CalcMobility(white, black);
    }
}

bool Board::IsLegal(uint64 pos)
{
    return (GetMobility() & pos) != 0;
}

bool Board::IsFinished()
{
    if (nbPlayed >= HIST_LENGTH)
    {
        return true;
    }
    return (CalcMobility(black, white) == 0) && (CalcMobility(white, black) == 0);
}
