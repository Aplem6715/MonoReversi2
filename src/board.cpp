#include <stdio.h>
#include <stdlib.h>
#include "board.h"
#include "bit_operation.h"

Board::Board(/* args */){};
Board::~Board(){};

uint64_t Board::GetBlack() { return black; }
uint64_t Board::GetWhite() { return white; }

uint64_t Board::GetOwn()
{
    if (turn == Const::BLACK)
    {
        return black;
    }
    return white;
}

uint64_t Board::GetOpp()
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
    black = 0x0000000810000000;
    white = 0x0000001008000000;
    turn = Const::BLACK;
    nbPlayed = 0;
}

uint64_t Board::PutTT(uint8 pos)
{
    uint64_t flip;
    if (turn == Const::BLACK)
    {
        flip = CalcFlip64(black, white, pos);
        black = black ^ flip ^ CalcPosBit(pos);
        white = white ^ flip;
    }
    else
    {
        flip = CalcFlip64(white, black, pos);
        black = black ^ flip;
        white = white ^ flip ^ CalcPosBit(pos);
    }
    // 着手情報を保存（どっちが，どこに打ち，どこを反転させたか）
    history[nbPlayed].color = turn;
    history[nbPlayed].pos = pos;
    history[nbPlayed].flip = flip;
    nbPlayed++;
    turn ^= 1;
    return flip;
}

uint8 Board::GetRandomPosMoveable()
{
    uint64_t mob = GetMobility();
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

int Board::Undo()
{
    uint64_t flip, pos;
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
        return 0;
    }
    return 1;
}

void Board::UndoUntilColorChange()
{
    uint8 prev_turn = turn;
    while (prev_turn == turn)
    {
        Undo();
    }
}

void Board::Skip()
{
    turn ^= 1;
}

void Board::Draw(uint64_t black, uint64_t white, uint64_t mobility)
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

void Board::Draw()
{
    Draw(black, white, GetMobility());
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

uint64_t Board::GetMobility()
{
    return GetMobility(turn);
}

uint64_t Board::GetMobility(uint8 color)
{
    if (color == Const::BLACK)
    {
        return CalcMobility64(black, white);
    }
    else
    {
        return CalcMobility64(white, black);
    }
}

bool Board::IsLegalTT(uint8 pos)
{
    return (GetMobility() & CalcPosBit(pos)) != 0;
}

bool Board::IsFinished()
{
    if (nbPlayed >= HIST_LENGTH)
    {
        return true;
    }
    return (CalcMobility64(black, white) == 0) && (CalcMobility64(white, black) == 0);
}
