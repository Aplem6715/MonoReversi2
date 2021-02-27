#if !defined(_SELFPLAY_H_)
#define _SELFPLAY_H_

extern "C"
{
#include "../const.h"
#include "../search/search.h"
}

#include <vector>
#include "game_record.hpp"

using namespace std;

uint8 PlayOneGame(vector<FeatureRecord> &featRecords, SearchTree *treeBlack, SearchTree *treeWhite, uint8 randomTurns, double randMoveRatio, double secondMoveRatio, bool useRecording);

void SelfPlay(uint8 midDepth, uint8 endDepth, bool resetWeight, vector<FeatureRecord> &testRecord);

#endif // _SELFPLAY_H_
