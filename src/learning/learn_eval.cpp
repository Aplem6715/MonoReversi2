#include "learn_eval.h"
#include "model.h"

float EvalTinyDnn(SearchTree *tree, uint8 nbEmpty)
{
    return tree->model->predict(tree->eval->FeatureStates, nbEmpty);
}