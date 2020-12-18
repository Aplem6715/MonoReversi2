#ifndef MODEL_DEFINED
#define MODEL_DEFINED

#include "const.h"
#include "dnn_io.h"

#pragma warning(push)
#pragma warning(disable : 4267)
#include "tiny_dnn/tiny_dnn.h"
#pragma warning(pop)

using namespace tiny_dnn;

class ACModel
{
private:
    network<graph> net;

public:
    ACModel(net_phase mode);
    ~ACModel(){};

    void predict(uint64 own, uint64 opp, uint64 mob, float &outValue, vec_t &outPolicy);
    void train(const std::vector<const GameData> &gameData);
};

#endif