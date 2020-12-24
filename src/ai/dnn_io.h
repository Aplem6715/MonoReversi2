
#include "const.h"

#pragma warning(push)
#pragma warning(disable : 4267)
#include "tiny_dnn/tiny_dnn.h"
#include "tiny_dnn/util/aligned_allocator.h"
#pragma warning(pop)

using namespace tiny_dnn;

typedef struct StateInfo
{
    uint64 own, opp;
    uint64 mob;
} StateInfo;

typedef struct GameData
{
    StateInfo state;
    float z;
    vec_t p;
} GameData;

void Game2TrainDataset(
    const std::vector<const GameData> &gameData,
    std::vector<std::vector<tensor_t>> &x,
    std::vector<std::vector<tensor_t>> &y);

void ConvertBoard(uint64 board, std::vector<std::vector<float_t, tiny_dnn::aligned_allocator<float_t, 64>>> &out);
