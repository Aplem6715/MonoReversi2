#include <stdio.h>
#include <Windows.h>
#include <assert.h>
#include "board.h"
#include "search/search.h"
#include <time.h>

#define PIPE1 "\\\\.\\pipe\\monoPipe1"
#define PIPE2 "\\\\.\\pipe\\monoPipe2"

uint8 WaitMoveResponse(HANDLE pipe)
{
    char buff[100];
    DWORD readedSize;
    if (!ReadFile(pipe, buff, sizeof(buff), &readedSize, NULL))
    {
        fprintf(stderr, "Couldn't read NamedPipe.\n");
    }
    return buff[0] - 1;
}

void SendMove(HANDLE pipe, uint8 pos)
{
    char buff[1];
    buff[0] = pos + 1;
    DWORD writtenSize;

    if (!WriteFile(pipe, buff, strlen(buff), &writtenSize, NULL))
    {
        fprintf(stderr, "Couldn't write NamedPipe.\n");
    }
    if (writtenSize != 1)
    {
        printf("written %d bytes\n", writtenSize);
    }
}

void Match(HANDLE pipe, uint8 myColor)
{
    SearchTree tree[1];
    uint64_t flip;
    uint8 pos;
    int nbEmpty = 60;
    Board board;

    InitTree(tree, 10, 16, 4, 8, 1);
    board.Reset();
    while (!board.IsFinished())
    {
        board.Draw();

        // 置ける場所がなかったらスキップ
        if (board.GetMobility() == 0)
        {
            board.Skip();
            continue;
        }

        if (board.GetTurnColor() == myColor)
        {
            printf("※考え中・・・\r");
            _sleep(750);
            // AIが着手位置を決める
            pos = Search(tree, board.GetOwn(), board.GetOpp(), 0);
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
            //printf("\n%f\n", treeWhite->score);
        }

        printf("pos: %d\n", pos);
        // 合法手判定
        if (!board.IsLegalTT(pos))
        {
            printf("error!!!!!! iligal move!!\n");
            return;
        }

        // 実際に着手
        flip = board.PutTT(pos);
        nbEmpty--;

    } //end of loop:　while (!board.IsFinished())
}

int main()
{
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    uint8 myColor = Const::WHITE;
    hPipe = CreateFile(TEXT(PIPE1),
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        myColor = Const::BLACK;
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
    if (myColor == Const::BLACK)
    {
        printf("黒 として接続\n");
    }
    else
    {
        printf("白 として接続\n");
    }

    srand((unsigned int)time(NULL));
    HashInit();

    Match(hPipe, myColor);

    CloseHandle(hPipe);
}