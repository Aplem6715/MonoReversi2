
#include "ascii_loader.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <assert.h>

extern "C"
{
#include "../board.h"
#include "../ai/eval.h"
#include "../bit_operation.h"
}

using namespace std;

void ConvertAsciiOnegame(vector<FeatureRecord> &featRecords, char predMoves[60], char bestMoves[60], int result)
{
    size_t topOfAddition = featRecords.size();
    uint64_t flip;
    uint8 pos;
    int nbEmpty = 60;
    int nbMoves = 0;
    bool readingAnalyzedMove = false;
    Board board[1];
    Evaluator eval[2];
    FeatureRecord record;
    int readingIndex = 0;

    BoardReset(board);
    EvalReload(&eval[0], BoardGetBlack(board), BoardGetWhite(board), OWN);
    EvalReload(&eval[1], BoardGetWhite(board), BoardGetBlack(board), OPP);
    while (!BoardIsFinished(board))
    {
        //BoardDraw(board);
        // 置ける場所がなかったらスキップ
        if (BoardGetMobility(board) == 0)
        {
            EvalUpdatePass(&eval[0]);
            EvalUpdatePass(&eval[1]);
            BoardSkip(board);
            continue;
        }

        // 入力位置のインデックスを取得
        if (readingAnalyzedMove)
        {
            pos = PosIndexFromAscii(&bestMoves[readingIndex]);
            readingIndex += 2;
        }
        else
        {
            pos = PosIndexFromAscii(&predMoves[readingIndex]);
            readingIndex += 2;
            if (predMoves[readingIndex] == '\0')
            {
                readingIndex = 0;
                readingAnalyzedMove = true;
            }
        }
        nbMoves++;

        // 合法手判定
        assert(BoardIsLegalTT(board, pos));

        // 実際に着手
        flip = BoardPutTT(board, pos);
        nbEmpty--;
        EvalUpdate(&eval[0], pos, flip);
        EvalUpdate(&eval[1], pos, flip);

        //黒用レコード設定
        for (int featIdx = 0; featIdx < FEAT_NUM; featIdx++)
        {
            record.featStats[OWN][featIdx] = eval[0].FeatureStates[featIdx];
            record.featStats[OPP][featIdx] = eval[1].FeatureStates[featIdx];
            assert(record.featStats[OWN][featIdx] < FTYPE_INDEX_MAX[FeatID2Type[featIdx]]);
            assert(record.featStats[OPP][featIdx] < FTYPE_INDEX_MAX[FeatID2Type[featIdx]]);
            assert(record.featStats[OPP][featIdx] == OpponentIndex(eval[0].FeatureStates[featIdx], FTYPE_DIGIT[FeatID2Type[featIdx]]));
        }
        record.nbEmpty = nbEmpty;
        record.stoneDiff = 1;
        record.color = BLACK;
        // レコードを追加
        featRecords.push_back(record);
        assert(CountBits(~(BoardGetBlack(board) | BoardGetWhite(board))) == nbEmpty);

    } //end of loop:　while (!BoardIsFinished(board))
    // 勝敗を表示
    //BoardDraw(board);
    int numBlack = BoardGetStoneCount(board, BLACK);
    int numWhite = BoardGetStoneCount(board, WHITE);
    signed char stoneDiff = numBlack - numWhite;
    assert(stoneDiff == result);

    // レコード終端から，リザルトが未確定のレコードに対してリザルトを設定
    for (; topOfAddition < featRecords.size(); topOfAddition++)
    {
        featRecords[topOfAddition].stoneDiff *= stoneDiff;
        if (numBlack > numWhite)
        {
            featRecords[topOfAddition].stoneDiff += WIN_BONUS;
        }
        else if (numWhite > numBlack)
        {
            featRecords[topOfAddition].stoneDiff -= WIN_BONUS;
        }
    }
}

void ConvertAscii2Feat(vector<FeatureRecord> &featRecords, string asciiFileName)
{
    string str;

    int index;
    int result;
    char predMoves[60 * 2];
    char bestMoves[60 * 2];

    ifstream asciiFile(asciiFileName);

    while (getline(asciiFile, str))
    {
        sscanf_s(str.c_str(),
                 "%d %s %s %d",
                 &index,
                 predMoves, (unsigned)sizeof(predMoves),
                 bestMoves, (unsigned)sizeof(bestMoves),
                 &result);
        ConvertAsciiOnegame(featRecords, predMoves, bestMoves, result);
    }
}
