#include "model.h"
#include "bit_operation.h"
#include "../ai/eval.h"
#include "../ai/ai_const.h"
#include <vector>

using namespace std;
using namespace tiny_dnn::layers;
using namespace tiny_dnn::activation;

void MakeInput(vec_t &input, const uint16 features[])
{
    int idxShift = 0;
    for (int i = 0; i < FEAT_NUM; i++)
    {
        input[idxShift + features[i]] = 1.0;
        idxShift += FeatMaxIndex[i];
    }
}

ValueModel::ValueModel(net_phase mode)
{
    for (int i = 0; i < NB_PHASE; i++)
    {
        network<sequential> net;
        net << fc(FEAT_NB_COMBINATION, VALUE_HIDDEN_UNITS1, false) << relu()
            << fc(VALUE_HIDDEN_UNITS1, VALUE_HIDDEN_UNITS2) << relu()
            << fc(VALUE_HIDDEN_UNITS2, 1) << linear(1);
        this->nets.push_back(net);
    }

    std::ofstream ofs("graph_net_example.txt");
    graph_visualizer viz(this->nets[0], "graph");
    viz.generate(ofs);
};

float ValueModel::predict(uint16 features[], uint8 nbEmpty)
{
    // 3 x 8 x 8
    static vector<vec_t> inputs(1, vec_t(FEAT_NB_COMBINATION));
    static float output;

    int idxShift = 0;

    MakeInput(inputs[0], features);

    //DEBUG
    for (int i = 0; i < FEAT_NUM; i++)
    {
        uint32 idx = 0;
        for (idx = 0; idx < FeatMaxIndex[i]; idx++)
        {
            if (inputs[0][idxShift + idx] > 0)
            {
                printf("%d, ", idx);
                break;
            }
        }
        idxShift += FeatMaxIndex[i];
    } //DEBUG

    output = nets[PHASE(nbEmpty)].predict(inputs)[0][0];
    return output;
}

void ValueModel::train(const std::vector<GameRecord> &gameRecords)
{
    adam opt;
    vector<tensor_t> input(NB_PHASE, tensor_t(gameRecords.size(), vec_t(FEAT_NUM)));
    vector<tensor_t> desiredOut(NB_PHASE, tensor_t(gameRecords.size()));
    size_t batchSize = 32;
    int epochs = 10;

    for (int i = 0; i < gameRecords.size(); i++)
    {
        int phase = PHASE(gameRecords[i].nbEmpty);
        MakeInput(input[phase][i], gameRecords[i].featStats);
        desiredOut[phase][i][0] = gameRecords[i].resultForBlack;
    }

    for (int phase = 0; phase < NB_PHASE; phase++)
    {
        nets[phase].fit<mse>(opt, input[phase], desiredOut[phase], batchSize, epochs);
    }
}

void ValueModel::Save(string saveDir)
{
    for (int phase = 0; phase < NB_PHASE; phase++)
    {
        nets[phase].save(saveDir + "phase" + to_string(phase));
    }
}

void ValueModel::Load(string loadDir)
{
    for (int phase = 0; phase < NB_PHASE; phase++)
    {
        nets[phase].load(loadDir + "phase" + to_string(phase));
    }
}
