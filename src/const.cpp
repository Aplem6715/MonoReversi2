#include "const.h"

const uint8 Const::BLACK = 0;
const uint8 Const::WHITE = 1;
const uint8 Const::EMPTY = 2;

// 位置指定とかぶらないよう，2bit以上立っているように
const uint8 Const::PASS = 0x11;
const uint8 Const::UNDO = 0x111;