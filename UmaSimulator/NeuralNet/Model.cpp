#include "Model.h"

ModelOutputValueV1::ModelOutputValueV1():scoreMean(0),scoreStdev(0),value(0)
{
}

ModelOutputValueV1::ModelOutputValueV1(float mean, float stdev, float v) :scoreMean(mean), scoreStdev(stdev), value(v)
{
}
