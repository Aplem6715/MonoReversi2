#ifndef MODEL_DEFINED
#define MODEL_DEFINED

#include "const.h"
#include "tiny_dnn/tiny_dnn.h"

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