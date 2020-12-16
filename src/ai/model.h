#ifndef MODEL_DEFINED
#define MODEL_DEFINED

#include "const.h"
#include "tiny_dnn/tiny_dnn.h"

using namespace tiny_dnn;

class ACModel
{
private:
    network<graph> net;

public:
    ACModel(net_phase mode);
    ~ACModel(){};

    void predict(uint64 own, uint64 opp, uint64 mob, float *outValue, float *outPolicy);
    void train();
};

#endif