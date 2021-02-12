
extern "C"
{
#include "game_record.hpp"
}
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