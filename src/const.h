#ifndef CONST_DEFINED
#define CONST_DEFINED

#include <stdbool.h>
#include <stdint.h>

/* stdintの整数型へ変更
typedef unsigned long long uint64_t;
typedef unsigned int uint32;
typedef unsigned short uint16;
*/
typedef int16_t score_t;
typedef int16_t score_strict_t;
typedef unsigned char uint8;

// 1石の価値 (signed 16bit)
#define STONE_VALUE (100)
// 評価関数の最大値 (50 x 64[stone])
#define EVAL_MAX (6400)
#define EVAL_MIN (-6400)
// 探索スコアの最大値 (EVAL_MAX + 1)
#define SCORE_MAX (6500)
#define SCORE_MIN (-6500)

#define MAX_VALUE INT16_MAX
/*
typedef float score_t;
typedef float score_strict_t;

// 1石の価値 (signed 16bit)
#define STONE_VALUE (10.0f)
// 評価関数の最大値 (500 x 64[stone])
#define EVAL_MAX (640.0f)
#define EVAL_MIN (-640.0f)
// 探索スコアの最大値 (EVAL_MAX + 1)
#define SCORE_MAX (650.0f)
#define SCORE_MIN (-650.0f)
*/

extern const uint8 BLACK;
extern const uint8 WHITE;
extern const uint8 EMPTY;

extern const uint8 PASS;
extern const uint8 UNDO;

extern const uint8 BOARD_SIZE;

#define NOMOVE_INDEX 64
#define PASS_INDEX 65

// 1ターンでの最大着手可能位置数
#define MAX_MOVES 32
#define OWN 0
#define OPP 1

// 座標インデックス
#define A1 63
#define B1 62
#define C1 61
#define D1 60
#define E1 59
#define F1 58
#define G1 57
#define H1 56

#define A2 55
#define B2 54
#define C2 53
#define D2 52
#define E2 51
#define F2 50
#define G2 49
#define H2 48

#define A3 47
#define B3 46
#define C3 45
#define D3 44
#define E3 43
#define F3 42
#define G3 41
#define H3 40

#define A4 39
#define B4 38
#define C4 37
#define D4 36
#define E4 35
#define F4 34
#define G4 33
#define H4 32

#define A5 31
#define B5 30
#define C5 29
#define D5 28
#define E5 27
#define F5 26
#define G5 25
#define H5 24

#define A6 23
#define B6 22
#define C6 21
#define D6 20
#define E6 19
#define F6 18
#define G6 17
#define H6 16

#define A7 15
#define B7 14
#define C7 13
#define D7 12
#define E7 11
#define F7 10
#define G7 9
#define H7 8

#define A8 7
#define B8 6
#define C8 5
#define D8 4
#define E8 3
#define F8 2
#define G8 1
#define H8 0

#endif