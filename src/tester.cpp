#include <stdio.h>

#include "./search/search.h"
#include "board.h"
#include "bit_operation.h"

#define LOG_FILE "./resources/tester/accurate.txt"

char records[10][61] = {
    "F5D6C5F4E3C6D3F6E6D7",
    "F5D6C3D3C4F4C5B3C2E3",
    "F5D6C3D3C4F4C5B3C2B4",
    "F5D6C3D3C4F4C5B3C2E6",
    "F5D6C3D3C4F4F6F3E6E7",
    "F5D6C4D3C5F4E3F3C2C6",
    "F5D6C4D3C5F4E3F3E2C6",
    "F5D6C4G5C6C5D7D3B4C3",
    "F5D6C4G5F6F4F3D3C3G6",
    "F5D6C4D3E6F4E3F3C6F6",
};

int Match(char *record, FILE *logFile)
{
    SearchTree tree[2];
    uint64_t flip;
    int nbEmpty = 60;
    Board board;

    uint8 pos;
    char posX;
    int posY;

    board.Reset();

    int i = 0;
    while (record[i] != '\0')
    {
        pos = CalcPosIndex(&record[i]);
        board.PutTT(pos);
        CalcPosAscii(pos, posX, posY);
        fprintf(logFile, "%c%d", posX, posY);
        i += 2;
    }

    fprintf(logFile, " ");

    while (!board.IsFinished())
    {
        board.Draw();

        // 置ける場所がなかったらスキップ
        if (board.GetMobility() == 0)
        {
            board.Skip();
            continue;
        }

        pos = Search(&tree[board.GetTurnColor()], board.GetOwn(), board.GetOpp(), 0);

        // 合法手判定
        if (!board.IsLegalTT(pos))
        {
            printf("error!!!!!! iligal move!!\n");
            return 0;
        }

        // 実際に着手
        flip = board.PutTT(pos);

        CalcPosAscii(pos, posX, posY);
        fprintf(logFile, "%c%d", posX, posY);
        nbEmpty--;

    } //end of loop:　while (!board.IsFinished())

    fprintf(logFile, "\n");
    return board.GetStoneCount(Const::BLACK) - board.GetStoneCount(Const::WHITE);
}

int main()
{

    FILE *fp = fopen(LOG_FILE, "w");
    int i = 0;

    for (i = 0; i < 10; i++)
    {
        Match(records[i], fp);
    }

    fclose(fp);

    getchar();
    getchar();
    return 0;
}