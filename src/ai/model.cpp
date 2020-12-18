#include "model.h"
#include "bit_operation.h"
#include <vector>

using namespace tiny_dnn::layers;
using namespace tiny_dnn::activation;

ACModel::ACModel(net_phase mode)
{
    static const uint8 bSize = Const::BOARD_SIZE;
    static const size_t win = 3;
    static const size_t nb_filter = 48;

    static const size_t p_win = 1;
    static const size_t p_nb_filter = 2;

    static const size_t v_win = 1;
    static const size_t v_nb_filter = 1;
    static const size_t v_hidden = 128;

    // intpu_ch: Own, Opp, Empty
    input in(shape3d(bSize, bSize, 3));

    conv conv_layer1(bSize, bSize, win, 3, nb_filter, padding::same);
    conv conv_layer2(bSize, bSize, win, nb_filter, nb_filter, padding::same);
    conv conv_layer3(bSize, bSize, win, nb_filter, nb_filter, padding::same);
    batch_norm bn1(conv_layer1);
    batch_norm bn2(conv_layer2);
    batch_norm bn3(conv_layer3);
    activation::leaky_relu activation1;
    activation::leaky_relu activation2;
    activation::leaky_relu activation3;

    auto &mid = in
                << conv_layer1 << bn1 << activation1
                << conv_layer2 << bn2 << activation2
                << conv_layer3 << bn3 << activation3;

    conv conv_p1(bSize, bSize, p_win, nb_filter, p_nb_filter, padding::same);
    conv conv_v1(bSize, bSize, v_win, nb_filter, v_nb_filter, padding::same);
    batch_norm bn_p1(conv_p1);
    batch_norm bn_v1(conv_v1);
    activation::leaky_relu activation_p1;
    activation::leaky_relu activation_v1;
    activation::leaky_relu activation_v2;

    fc fc_v2(bSize * bSize * v_nb_filter, v_hidden);
    fc outPolicy_fc(bSize * bSize * p_nb_filter, bSize * bSize);
    fc outValue_fc(v_hidden, 1);

    softmax activation_policy;
    tanh_layer activation_value(1);

    conv_p1 << bn_p1 << activation_p1
            << outPolicy_fc << activation_policy;

    conv_v1 << bn_v1 << activation_v1
            << fc_v2 << activation_v2
            << outValue_fc << activation_value;

    // policyネット部
    mid << conv_p1;
    // valueネット部
    mid << conv_v1;

    construct_graph(this->net, {&in}, {&activation_policy, &activation_value});
    this->net.set_netphase(mode);

    std::ofstream ofs("graph_net_example.txt");
    graph_visualizer viz(this->net, "graph");
    viz.generate(ofs);
};

void ACModel::predict(uint64 own, uint64 opp, uint64 mob, float &outValue, vec_t &outPolicy)
{
    // 3 x 8 x 8
    static std::vector<tensor_t> inputs(3, tensor_t(8, vec_t(8)));
    static tiny_dnn::tensor_t output;
    ConvertBoard(own, inputs[0]);
    ConvertBoard(opp, inputs[1]);
    ConvertBoard(mob, inputs[2]);

    output = net.predict(inputs)[0];
    for (int i = 0; i < 64; i++)
    {
        outPolicy = output[0];
    }
    outValue = output[1][0];
}

void ACModel::train(const std::vector<const GameData> &gameData)
{
    adam opt;
    std::vector<tensor_t> input;
    std::vector<tensor_t> desiredOut;
    size_t batchSize = 64;
    int epochs = 10;

    net.fit<mse>(opt, input, desiredOut, batchSize, epochs);
}