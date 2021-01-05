#include "game_record.h"
#include <stdlib.h>

void sampling(FeatureRecord **records, FeatureRecord **sampledList, int nbRecords, int nbSample)
{
    int i;
    uint32 randIdx;
    for (i = 0; i < nbSample; i++)
    {
        // 0~100万の乱数を作ってレコード数で丸め
        randIdx = ((rand() % 1000) * 1000 + (rand() % 1000)) % nbRecords;
        sampledList[i] = records[randIdx];
    }
}