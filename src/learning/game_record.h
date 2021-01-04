#ifndef GAME_RECORD_DEFINED
#define GAME_RECORD_DEFINED

#include "../const.h"

#define FEAT_NUM 46
#define RESULT_UNSETTLED -100

typedef struct FeatureRecord
{
    uint16 featStats[FEAT_NUM];
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
    uint32 nbRecords;
    uint16 tournamentYear;
    uint8 boardSize;
    uint8 gameType;
    uint8 depth;
    uint8 padding;
} WthorHeaderWTB;

typedef struct WthorWTB
{
    uint16 tourIdx;
    uint16 blackPlayerIdx;
    uint16 whitePlayerIdx;
    uint8 nbBlackStone;
    uint8 nbBlackAnalyzed;
    uint8 moves88[60];
} WthorWTB;

#endif