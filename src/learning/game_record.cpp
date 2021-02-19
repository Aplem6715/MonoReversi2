/**
 * @file game_record.cpp
 * @author Daichi Sato
 * @brief 対戦結果からいくつかのデータを取得する
 * @version 1.0
 * @date 2021-02-12
 * 
 * @copyright Copyright (c) 2021 Daichi Sato
 * 
 * メインはヘッダーにあるGameRecord関連の構造体
 * sampling関数はおまけの便利機能的位置
 * 
 */

extern "C"
{
#include "../const.h"
#include "../ai/ai_const.h"
}

#include "game_record.hpp"
#include <stdlib.h>
#include <vector>

using namespace std;

void sampling(vector<FeatureRecord *> &records, FeatureRecord **sampledList, int nbSample)
{
    int i;
    uint32_t randIdx;

    for (i = 0; i < nbSample; i++)
    {
        // 0~100万の乱数を作ってレコード数で丸め
        randIdx = ((rand() % 1000) * 1000 + (rand() % 1000)) % records.size();
        sampledList[i] = records[i];
        records.erase(records.begin() + i);
    }
}