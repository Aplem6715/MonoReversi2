/**
 * @file tester.c
 * @author Daichi Sato
 * @brief 探索の変更前後で着手位置が変化していないかをテストする（未実装）
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "./search/search.h"
#include "board.h"
#include "bit_operation.h"

#define LOG_FILE "./resources/tester/accurate_endRootHashOrder_nohash.txt"

#define NB_RECORDS 19
#define NB_RANDOM_TURN 40

char records[NB_RECORDS][61 * 2] = {
    "F5D6C5F4E3F6",
    //"F5D6C6",
    "F5D6C5F4D3",
    "F5D6C5F4D7",
    "F5D6C5F4E3D3",
    "F5D6C5F4E3F6",
    "F5D6C5F4E3C6E6",
    "F5D6C5F4E3C6F3",
    "F5D6C5F4E3C6D3G5",
    "F5D6C5F4E3C6D3F3",
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

/*
char records[NB_RECORDS][61 * 2] = {
    "D3C5B6F3F4B5E6F5C6B7F6D7G4C2D6G6C7C3C8B8B1E8A8D2F7B2B3G3A5H3E7F8D1E3G5C4A7B4H6G7",
    "F5D6C3D3C4F4C5B3C2E3D2C6B4A3G4E1D1C1E2G6B5E6A4F1F7G3F3F6H3H4A2E8H5F2B1A1B2B6A5A6G5H6H7G2H1H2G7",
    "F5D6C3D3C4F4C5B3C2B4E3E6B6C6B5A4A6C1A2G4G6G5C7D7F6F7D8E7H5H4H3H6G3E2F3D2F2H2G8F8E8A3A5E1G2C8B8",
    "F5D6C3D3C4F4C5B3C2E6B4F6C6B6G4B5A6A5A3A7G6C1E7G5D2C7F7E8D7D8F8G8B8H3H5D1E3G3H4F3E2E1F2H7F1H6B1",
    "F5D6C3D3C4F4F6F3E6E7C6G5D2D7F8F7E8C8E3C5G8C7B4B5G3A3G4H3A6A5B6F2H5H4H6G6H2D8B8G7A4E2A2C1G1F1E1",
    "F5D6C4D3C5F4E3F3C2C6E6D2G4B6B5C3B4C1F2A6A5B3A7F6A4A3A2E2E7D7G5G6F7F8H7G3H4F1B2H5H6H2C8D8E8A1G8",
};

* /
/**
 * @brief 対戦の実行
 * 
 * @param record 着手位置インデックスの記録
 * @param logFile 出力ファイル
 * @return int 終了ステータス
 */
int Match(char *record, SearchTree tree[2], FILE *logFile)
{
    uint64_t flip;
    int nbEmpty = 60;
    Board board[1];

    uint8 pos;
    char posX;
    int posY;

    BoardReset(board);

    int i = 0;
    int record_i = 0;
    if (record != NULL)
    {
        // レコード着手
        while (record[record_i] != '\0')
        {
            //BoardDraw(board);
            pos = PosIndexFromAscii(&record[record_i]);

            if (!BoardIsLegalTT(board, pos))
            {
                printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!\n 不正な着手位置1\n");
                return -1;
            }

            BoardPutTT(board, pos);
            CalcPosAscii(pos, &posX, &posY);
            //fprintf(logFile, "%c%d\n", posX, posY);
            fprintf(logFile, "%c%d", posX, posY);
            record_i += 2;
            nbEmpty--;
        }
    }
    else
    {
        // ランダム着手
        for (i = 0; i < NB_RANDOM_TURN; i++)
        {
            if (BoardGetMobility(board) == 0)
            {
                BoardSkip(board);
                continue;
            }

            //BoardDraw(board);
            pos = BoardGetRandomPosMoveable(board);

            if (!BoardIsLegalTT(board, pos))
            {
                printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!\n 不正な着手位置2\n");
                return -1;
            }

            BoardPutTT(board, pos);
            CalcPosAscii(pos, &posX, &posY);
            //fprintf(logFile, "%c%d\n", posX, posY);
            fprintf(logFile, "%c%d", posX, posY);
            nbEmpty--;
        }
    }

    fprintf(logFile, " ");

    while (!BoardIsFinished(board))
    {
        //BoardDraw(board);

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
            return -1;
        }

        // 実際に着手
        flip = BoardPutTT(board, pos);

        CalcPosAscii(pos, &posX, &posY);
        //fprintf(logFile, "%c%d %d\n", posX, posY, tree[BoardGetTurnColor(board)].score);
        fprintf(logFile, "%c%d", posX, posY);
        nbEmpty--;

    } //end of loop:　while (!BoardIsFinished(board))

    fprintf(logFile, " %d\n", BoardGetStoneCount(board, BLACK) - BoardGetStoneCount(board, WHITE));
    return 0;
}

int main()
{
    SearchTree tree[2];
    FILE *fp = fopen(LOG_FILE, "w");
    int i = 0;

    srand(42);
    HashInit();

    InitTree(&tree[0], 6, 14, 4, 8, 0, 0, 0, 0);
    InitTree(&tree[1], 6, 14, 4, 8, 0, 0, 0, 0);
    // 設定上書き
    tree[0].useIDDS = 1;
    tree[1].useIDDS = 1;

    tree[0].hashDepth = 4;
    tree[1].hashDepth = 4;

    tree[0].orderDepth = 5;
    tree[1].orderDepth = 5;

    for (i = 0; i < 100; i++)
    {
        ResetTree(&tree[0]);
        ResetTree(&tree[1]);

        printf("match %d\n", i);
        if (i < NB_RECORDS)
        {
            if (Match(records[i], tree, fp) != 0)
            {
                break;
            }
        }
        else
        {
            if (Match(NULL, tree, fp) != 0)
            {
                break;
            }
        }
    }

    fclose(fp);

    getchar();
    getchar();
    return 0;
}