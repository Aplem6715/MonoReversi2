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

#define LOG_FILE "./resources/tester/accurate_nompc_fullhash_beforeRootOrder_fixPVS_aftermidFix.txt"

#define NB_RECORDS 19
#define NB_RANDOM_TURN 10

char records[NB_RECORDS][61] = {
    "F5D6C6",
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
            BoardDraw(board);
            pos = PosIndexFromAscii(&record[record_i]);

            if (!BoardIsLegalTT(board, pos))
            {
                printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!\n 不正な着手位置\n");
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
            BoardDraw(board);
            pos = BoardGetRandomPosMoveable(board);

            if (!BoardIsLegalTT(board, pos))
            {
                printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!\n 不正な着手位置\n");
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
            return -1;
        }

        // 実際に着手
        flip = BoardPutTT(board, pos);

        CalcPosAscii(pos, &posX, &posY);
        //fprintf(logFile, "%c%d %d\n", posX, posY, tree->score);
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

    InitTree(&tree[0], 8, 12, 4, 8, 1, 1, 0, 0);
    InitTree(&tree[1], 8, 12, 4, 8, 1, 1, 0, 0);
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