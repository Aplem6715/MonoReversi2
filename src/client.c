/**
 * @file client.c
 * @author Daichi Sato
 * @brief クライアントとして動作する際のメイン部
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 * AI強化後の効果確認のためにサーバー上で対戦する。
 * 公開用ではなく確認用のため，通信などは適当な作りになっている。
 * 
 */

#include <stdio.h>
#include <Windows.h>
#include <assert.h>
#include "board.h"
#include "search/search.h"
#include <time.h>

#define PIPE1 "\\\\.\\pipe\\monoPipe1"
#define PIPE2 "\\\\.\\pipe\\monoPipe2"

#define ERROR_POS 127

#define MID_DEPTH 15
#define USE_MPC 1
#define USE_NEST_MPC 1

/**
 * @brief 盤面情報をサーバーからダウンロードする
 * 
 * @param pipe 通信パイプ
 * @param black 黒石情報への参照
 * @param white 白石情報への参照
 * @return uint8 手番の色
 */
uint8 BoardDownload(HANDLE pipe, uint64_t *black, uint64_t *white)
{
    uint64_t buff[5];
    DWORD readedSize;
    if (!ReadFile(pipe, buff, sizeof(uint64_t) * 5, &readedSize, NULL))
    {
        fprintf(stderr, "Couldn't read NamedPipe.\n");
    }
    *black = buff[0];
    *white = buff[1];
    return (uint8)buff[2];
}

/**
 * @brief サーバーからの着手を待機
 * 
 * @param pipe 通信パイプ
 * @return uint8 着手位置
 */
uint8 WaitMoveResponse(HANDLE pipe)
{
    char buff[1];
    DWORD readedSize;
    if (!ReadFile(pipe, buff, 1, &readedSize, NULL))
    {
        fprintf(stderr, "Couldn't read NamedPipe.\n");
        return ERROR_POS;
    }
    return buff[0] - 1;
}

/**
 * @brief 着手情報を送信
 * 
 * @param pipe 通信パイプ
 * @param pos 着手位置
 */
void SendMove(HANDLE pipe, uint8 pos)
{
    char buff[1];
    buff[0] = pos + 1;
    DWORD writtenSize;

    if (!WriteFile(pipe, buff, 1, &writtenSize, NULL))
    {
        fprintf(stderr, "Couldn't write NamedPipe.\n");
    }
    if (writtenSize != 1)
    {
        printf("written %d bytes\n", writtenSize);
    }
}

/**
 * @brief 対戦の実行
 * 
 * @param pipe 通信パイプ
 * @param myColor 自分の色
 * @param black 黒石情報
 * @param white 白石情報
 * @param turn スタート時点の手番色
 * @return int 正常終了0 エラー番号
 */
int Match(HANDLE pipe, uint8 myColor, uint64_t black, uint64_t white, uint8 turn)
{
    SearchTree tree[1];
    uint64_t flip;
    uint8 pos;
    int nbEmpty = 60;
    Board board[1];

    InitTree(tree, MID_DEPTH, 18, 4, 8, 1, USE_MPC, USE_NEST_MPC);
    BoardReset(board);
    BoardSetStones(board, black, white, turn);
    while (!BoardIsFinished(board))
    {
        BoardDraw(board);

        // 置ける場所がなかったらスキップ
        if (BoardGetMobility(board) == 0)
        {
            BoardSkip(board);
            continue;
        }

        if (BoardGetTurnColor(board) == myColor)
        {
            printf("※考え中・・・\r");
            // AIが着手位置を決める
            pos = Search(tree, BoardGetOwn(board), BoardGetOpp(board), 0);
            printf("思考時間：%.2f[s]  探索ノード数：%zu[Node]  探索速度：%.1f[Node/s]  推定CPUスコア：%.1f",
                   tree->usedTime, tree->nodeCount, tree->nodeCount / tree->usedTime, tree->score / (float)(STONE_VALUE));
            if (tree->isEndSearch)
            {
                printf("(WLD)");
            }
            printf("\n");
            SendMove(pipe, pos);
            printf("Sended\n");
            //printf("\n%f\n", treeBlack->score);
        }
        else
        {
            // AIが着手位置を決める
            pos = WaitMoveResponse(pipe);
            if (pos == ERROR_POS)
            {
                DeleteTree(tree);
                return ERROR_POS;
            }
            //printf("\n%f\n", treeWhite->score);
        }

        printf("pos: %d\n", pos);
        // 合法手判定
        if (!BoardIsLegalTT(board, pos))
        {
            printf("error!!!!!! iligal move!!\n");
            return ERROR_POS;
        }

        // 実際に着手
        flip = BoardPutTT(board, pos);
        nbEmpty--;

    } //end of loop:　while (!BoardIsFinished(board))
    DeleteTree(tree);
    return 0;
}

/**
 * @brief 通信対戦のメイン関数
 * 
 * 通信パイプを作成・接続し，エラー終了するまで対戦を繰り返す。
 * 動作確認用なのでそのへんは適当な作りになっている。
 * 
 * @return int 
 */
int main()
{
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    uint8 myColor = WHITE;
    hPipe = CreateFile(TEXT(PIPE1),
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        myColor = BLACK;
        hPipe = CreateFile(TEXT(PIPE2),
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           0,
                           NULL);
        if (hPipe == INVALID_HANDLE_VALUE)
        {
            fprintf(stderr, "Couldn't create NamedPipe.\n");
            return 1;
        }
    }
    if (myColor == BLACK)
    {
        printf("黒 として接続\n");
    }
    else
    {
        printf("白 として接続\n");
    }

    uint64_t black, white;
    uint8 turn;

    srand((unsigned int)time(NULL));
    HashInit();

    printf("Mid:%d MPC:%d", MID_DEPTH, USE_MPC);

    while (1)
    {
        turn = BoardDownload(hPipe, &black, &white);
        if (Match(hPipe, myColor, black, white, turn) == ERROR_POS)
        {
            // エラー終了
            break;
        }
    }

    getchar();
    getchar();
    CloseHandle(hPipe);
}