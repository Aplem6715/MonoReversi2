#if !defined(_ASCII_LOADER_H_)
#define _ASCII_LOADER_H_

#include <vector>
#include "game_record.hpp"

using namespace std;

void ConvertAscii2Feat(vector<FeatureRecord> &featRecords, string asciiFileName);

#endif // _ASCII_LOADER_H_
