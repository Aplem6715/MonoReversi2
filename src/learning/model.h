#ifndef MODEL_DEFINED
#define MODEL_DEFINED

#pragma warning(push)
#pragma warning(disable : 4267)
#include "tiny_dnn/tiny_dnn.h"
#pragma warning(pop)

#include "../const.h"
#include "../ai/eval.h"
#include "game_record.h"
#include <vector>
#include <string>

using namespace tiny_dnn;
using namespace std;

class ValueModel
{
private:
    vector<network<sequential>> nets;

public:
    ValueModel();
    ~ValueModel(){};

    float predict(uint16 features[], uint8 nbEmpty);
    void train(const std::vector<GameRecord> &gameRecords);
    void Save(string saveDir);
    void Load(string loadDir);
    void CopyWeightsTo(Weights weights[NB_PHASE][NB_LAYERS]);
};

#endif