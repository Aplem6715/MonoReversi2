#ifndef PVS_DEFINED
#define PVS_DEFINED

#define WIN_VALUE (1000000)

#include <time.h>

#include "hash.h"
#include "moves.h"
#include "../stones.h"
#include "../const.h"
#include "../ai/eval.h"

/**
 * @brief 探索木の情報を持つオブジェクト
 * 
 */
typedef struct SearchTree
{
    // NullWindowSearch用ハッシュ表
    HashTable *nwsTable;
    // PVノード用ハッシュ表
    HashTable *pvTable;

    // 評価オブジェクト
    Evaluator eval[1];
    // 石情報
    Stones stones[1];
    // 残り空きマス数
    uint8 nbEmpty;

    // 探索深度
    unsigned char depth;
    // move ordering 限界深度
    unsigned char orderDepth;
    // ハッシュ表限界深度
    unsigned char hashDepth;
    // PVハッシュの限界深度
    unsigned char pvHashDepth;
    // PVS限界深度
    unsigned char pvsDepth;

    // 中盤探索深度
    unsigned char midDepth;
    // 終盤探索深度
    unsigned char endDepth;
    // 中盤探索PVS限界
    unsigned char midPvsDepth;
    // 終盤探索PVS限界
    unsigned char endPvsDepth;

    // ハッシュ表を利用するかどうか
    bool useHash;
    bool usePvHash;
    // 反復深化を利用するかどうか
    bool useIDDS;
    // Multi Prob Cutを利用するかどうか
    bool useMPC;
    // MPCの探索内でさらにMPCを許可するかどうか
    bool enableMpcNest;
    // MPCの重複回数
    uint8 nbMpcNested;

    /* For Stats */
    // 探索ノード数
    size_t nodeCount;
    // ベータカット数
    size_t nbCut;
    // 探索時間
    double usedTime;
    // 予想最善手の探索スコア
    score_t score;
    // 終盤探索だったかどうか
    uint8 isEndSearch;

    // タイムリミットの有効・無効
    bool useTimeLimit;
    // 探索終了時刻
    clock_t timeLimit;
    // 中断されたか
    bool isIntrrupted;

    // CUIメッセージ利用時のバッファ
    char msg[1024];
} SearchTree;

typedef score_t (*SearchFunc_t)(SearchTree *tree, score_t alpha, score_t beta, unsigned char depth, bool passed);
typedef score_t (*SearchFuncNullWindow_t)(SearchTree *tree, score_t alpha, unsigned char depth, bool passed);

void InitTree(SearchTree *tree, unsigned char midDepth, unsigned char endDepth, unsigned char midPvsDepth, unsigned char endPvsDepth, bool useHash, bool usePvHash, bool useMPC, bool nestMPC);
void DeleteTree(SearchTree *tree);
void ConfigTree(SearchTree *tree, unsigned char midDepth, unsigned char endDepth);
void ResetTree(SearchTree *tree);

void SearchSetup(SearchTree *tree, uint64_t own, uint64_t opp);
void SearchPassMid(SearchTree *tree);
void SearchUpdateMid(SearchTree *tree, Move *move);
void SearchRestoreMid(SearchTree *tree, Move *move);
void SearchUpdateMidDeep(SearchTree *tree, uint64_t pos, uint64_t flip);
void SearchRestoreMidDeep(SearchTree *tree, uint64_t pos, uint64_t flip);

void SearchPassEnd(SearchTree *tree);
void SearchUpdateEnd(SearchTree *tree, Move *move);
void SearchRestoreEnd(SearchTree *tree, Move *move);
void SearchUpdateEndDeep(SearchTree *tree, uint64_t pos, uint64_t flip);
void SearchRestoreEndDeep(SearchTree *tree, uint64_t pos, uint64_t flip);

uint8 Search(SearchTree *tree, uint64_t own, uint64_t opp, bool choiceSecond);

#endif