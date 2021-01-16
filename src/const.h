#ifndef CONST_DEFINED
#define CONST_DEFINED

typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

struct Const
{
    static const uint8 WHITE;
    static const uint8 BLACK;
    static const uint8 EMPTY;

    static const uint8 PASS;
    static const uint8 UNDO;

    static const uint8 BOARD_SIZE;

    static const float MAX_VALUE;
};

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