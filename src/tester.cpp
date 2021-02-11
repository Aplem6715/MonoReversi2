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
    Board board[1];
    ;

    uint8 pos;
    char posX;
    int posY;

    BoardReset(board);

    int i = 0;
    while (record[i] != '\0')
    {
        pos = CalcPosIndex(&record[i]);
        BoardPutTT(board, pos);
        CalcPosAscii(pos, posX, posY);
        fprintf(logFile, "%c%d", posX, posY);
        i += 2;
    }

    fprintf(logFile, " ");

    while (!BoardIsFinished(board))
    {
        BoardDraw(board);

        // 置ける場所がなかったらスキップ
        if (BoardGetMobility(board) == 0)
        {
            BoardSkip(board);
            continue;
        }

        pos = Search(&tree[BoardGetTurnColor(board)], BoardGetOwn(board), BoardGetOpp(board), 0);

        // 合法手判定
        if (!BoardIsLegalTT(board, pos))
        {
            printf("error!!!!!! iligal move!!\n");
            return 0;
        }

        // 実際に着手
        flip = BoardPutTT(board, pos);

        CalcPosAscii(pos, posX, posY);
        fprintf(logFile, "%c%d", posX, posY);
        nbEmpty--;

    } //end of loop:　while (!BoardIsFinished(board))

    fprintf(logFile, "\n");
    return BoardGetStoneCount(board, BLACK) - BoardGetStoneCount(board, WHITE);
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