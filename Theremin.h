//
// LoopGrid by Martin Minovski, 2018
//

#ifndef RTPIANO_FSR_H
#define RTPIANO_FSR_H

#include <vector>
#include "LooperWidget.h"
using namespace std;

#define NUMBER_OF_INPUTS 7

class LooperWidget;

class Theremin {
public:
    static const int nInputs = NUMBER_OF_INPUTS;
    static vector<LooperWidget*> inputs[NUMBER_OF_INPUTS];
    static void clear();
    static void setValues(float* fsrValues);
};


#endif //RTPIANO_FSR_H
