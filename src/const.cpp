#include "const.h"
#include <float.h>

const uint8 Const::BLACK = 0;
const uint8 Const::WHITE = 1;
const uint8 Const::EMPTY = 2;

// 位置指定とかぶらないよう，2bit以上立っているように
const uint8 Const::PASS = 3;
const uint8 Const::UNDO = 7;

const uint8 Const::BOARD_SIZE = 8;

const float Const::MAX_VALUE = FLT_MAX;