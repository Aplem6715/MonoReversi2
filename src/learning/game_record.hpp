#ifndef GAME_RECORD_DEFINED
#define GAME_RECORD_DEFINED

extern "C"
{
#include "../const.h"
#include "../ai/ai_const.h"
}

#include <vector>
using namespace std;

#define RESULT_UNSETTLED -100

typedef struct FeatureRecord
{
    uint16_t featStats[2][FEAT_NUM];
    uint8 nbEmpty;
    uint8 color;
    signed char stoneDiff;
} FeatureRecord;

typedef struct GameRecord
{
    uint8 moves[60];
    uint8 stoneDiffBlack;
} GameRecord;

typedef struct WthorHeaderWTB
{
    uint8 cent;
    uint8 year;
    uint8 month;
    uint8 day;
    uint32_t nbRecords;
    uint16_t tournamentYear;
    uint8 boardSize;
    uint8 gameType;
    uint8 depth;
    uint8 padding;
} WthorHeaderWTB;

typedef struct WthorWTB
{
    uint16_t tourIdx;
    uint16_t blackPlayerIdx;
    uint16_t whitePlayerIdx;
    uint8 nbBlackStone;
    uint8 nbBlackAnalyzed;
    uint8 moves88[60];
} WthorWTB;

void sampling(vector<FeatureRecord *> &records, FeatureRecord **sampledList, int nbSample);

#endif