
/**
 * @file dll.c
 * @author Daichi Sato
 * @brief GUI用のDLL関数郡定義
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 * C#-WPFで作成したGUIプログラムにDLLとして読み込ませるための関数。
 * グローバル変数としてBoardと探索ツリー，ログ出力用のコールバックを保持する。
 * 
 */

#include <time.h>
#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include "search/search_manager.h"
#include "board.h"
#include "bit_operation.h"

#ifdef _WINDLL

#ifndef DLLAPI
#define DLLAPI __declspec(dllexport)
#endif

enum GUI_TextColor
{
    GUI_BLACK,
    GUI_WHITE,
    GUI_LIME,
    GUI_ORANGE,
    GUI_RED
};

typedef const void(__stdcall *GUI_Log)(int knd, char *str);

static GUI_Log GUI_Print;
//SearchTree dllTree[1];
static SearchManager sManager[1];
static Board dllBoard[1];
static uint8 aiColor;

DLLAPI void DllInit();
DLLAPI void DllConfigureSearch(unsigned char midDepth, unsigned char endDepth, int oneMoveTime, bool useTimer, bool useMPC, bool enablePreSearch);
DLLAPI int DllSearch(double *value);

DLLAPI void DllBoardReset();
DLLAPI int DllPut(int pos);
DLLAPI int DllStoneCount(int color);
DLLAPI int DllUndo();
DLLAPI int DllIsGameEnd();
DLLAPI int DllIsLegal(int pos);
DLLAPI uint64_t DllGetMobility();
DLLAPI uint64_t DllGetMobilityC(int color);
DLLAPI uint64_t DllGetStones(int color);

DLLAPI void SetCallBack(GUI_Log printCallback);
DLLAPI void DllShowMsg();

/**
 * @brief DLL内で使われる変数や機能などの初期化
 * 
 * 乱数シード設定
 * ハッシュ表のハッシュコード設定
 * 探索木の初期化
 * 盤面の初期化
 * 
 */
void DllInit()
{
    srand(GLOBAL_SEED);
    HashInit();
    //TreeInit(dllTree);
    SearchManagerInit(sManager, 4, true);
    BoardReset(dllBoard);

    FILE *fp;
    AllocConsole();
    freopen_s(&fp, "CONOUT$", "w", stdout); /* 標準出力(stdout)を新しいコンソールに向ける */
    freopen_s(&fp, "CONOUT$", "w", stderr); /* 標準エラー出力(stderr)を新しいコンソールに向ける */
}

/**
 * @brief 探索深度の設定を行う
 * 
 * @param midDepth 中盤探索深度
 * @param endDepth 終盤探索深度
 * @param oneMoveTime 一手にかける時間
 * @param useTimer 時間制限トグル
 * @param useMPC MPC利用トグル
 */
void DllConfigureSearch(unsigned char midDepth, unsigned char endDepth, int oneMoveTime, bool useTimer, bool useMPC, bool enablePreSearch)
{
    SearchManagerConfigure(sManager, midDepth, endDepth, oneMoveTime, true, useTimer, useMPC);
    sManager->enableAsyncPreSearching = enablePreSearch;
}

/**
 * @brief 予想最善手の探索を行う
 * 
 * @param value 評価値への参照
 * @return int 着手位置インデックス
 */
int DllSearch(double *value)
{
    score_t scoreMap[64];
    uint8 pos = SearchManagerGetMove(sManager, scoreMap);
    SearchManagerUpdateOwn(sManager, pos);
    *value = sManager->primaryBranch->tree->score;
    return pos;
}

/**
 * @brief 盤面の初期化
 * 
 */
void DllBoardReset()
{
    uint64_t own, opp;
    BoardReset(dllBoard);
    if (aiColor == BLACK)
    {
        own = BoardGetBlack(dllBoard);
        opp = BoardGetWhite(dllBoard);
    }
    else
    {
        own = BoardGetWhite(dllBoard);
        opp = BoardGetBlack(dllBoard);
    }
    SearchManagerReset(sManager, own, opp);
}

/**
 * @brief 着手
 * 
 * @param pos 着手位置
 * @return int 着手できたかどうかbool
 */
int DllPut(int pos)
{
    if (BoardIsLegalTT(dllBoard, pos))
    {
        BoardPutTT(dllBoard, pos);
        if (BoardGetMobility(dllBoard) == 0)
        {
            BoardSkip(dllBoard);
        }
        return 1;
    }
    return 0;
}

/**
 * @brief 石の数を計算
 * 
 * @param color 数える石の色
 * @return int 石の数
 */
int DllStoneCount(int color)
{
    return BoardGetStoneCount(dllBoard, color);
}

/**
 * @brief 一手戻す
 * 
 * @return int 実行結果bool 
 */
int DllUndo()
{
    if (BoardUndo(dllBoard))
    {
        return 1;
    }
    return 0;
}

/**
 * @brief 終局判定
 * 
 * @return int 終局していたら1のbool
 */
int DllIsGameEnd()
{
    return BoardIsFinished(dllBoard);
}

/**
 * @brief 着手が可能かどうか
 * 
 * @param pos 着手位置
 * @return int 着手可否bool
 */
int DllIsLegal(int pos)
{
    return BoardIsLegalTT(dllBoard, pos);
}

/**
 * @brief 手番の着手可能位置を取得
 * 
 * @return uint64_t 着手可能位置bit
 */
uint64_t DllGetMobility()
{
    return BoardGetMobility(dllBoard);
}

/**
 * @brief 指定色の着手可能位置を取得
 * 
 * @param color 指定色
 * @return uint64_t 着手可能位置bit
 */
uint64_t DllGetMobilityC(int color)
{
    return BoardGetColorsMobility(dllBoard, color);
}

/**
 * @brief 石情報を取得
 * 
 * @param color 取得する石情報の色
 * @return uint64_t 石情報bit
 */
uint64_t DllGetStones(int color)
{
    if (color == BLACK)
    {
        return BoardGetBlack(dllBoard);
    }
    else
    {
        return BoardGetWhite(dllBoard);
    }
}

/**
 * @brief コールバック関数を設定する
 * 
 * @param printCallback メッセージ表示時に呼び出される関数
 */
void SetCallBack(GUI_Log printCallback)
{
    GUI_Print = printCallback;
    GUI_Print(GUI_ORANGE, "System: Callback Initialized");
}

/**
 * @brief メッセージを表示する
 * 
 */
void DllShowMsg()
{
    GUI_Print(GUI_LIME, sManager->primaryBranch->tree->msg);
}
#endif