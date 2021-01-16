#ifndef CONST_DEFINED
#define CONST_DEFINED

typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

#define MAX_MOVES 32

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

#endif