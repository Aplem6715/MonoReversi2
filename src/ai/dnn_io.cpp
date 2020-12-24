#include "dnn_io.h"

void ConvertBoard(uint64 board, std::vector<std::vector<float_t, tiny_dnn::aligned_allocator<float_t, 64>>> &out)
{
    for (int y = 7; y >= 0; y--)
    {
        out[y] = {
            (float)((board >> (y * 8 + 7)) & 1),
            (float)((board >> (y * 8 + 6)) & 1),
            (float)((board >> (y * 8 + 5)) & 1),
            (float)((board >> (y * 8 + 4)) & 1),
            (float)((board >> (y * 8 + 3)) & 1),
            (float)((board >> (y * 8 + 2)) & 1),
            (float)((board >> (y * 8 + 1)) & 1),
            (float)((board >> (y * 8 + 0)) & 1),
        };
    }
}
