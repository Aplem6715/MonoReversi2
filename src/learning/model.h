#ifndef MODEL_DEFINED
#define MODEL_DEFINED

#include "../const.h"
#include "game_data.h"
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4267)
#include "tiny_dnn/tiny_dnn.h"
#pragma warning(pop)

using namespace tiny_dnn;

class ACModel
{
private:
    vector<network<sequential>> nets;

public:
    ACModel(net_phase mode);
    ~ACModel(){};

    float predict(uint16 features[], uint8 nbEmpty);
    void train(const std::vector<const GameData> &gameRecords);
};

#endif