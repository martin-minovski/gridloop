//
// Created by martin on 5/24/18.
//

#ifndef RTPIANO_FSR_H
#define RTPIANO_FSR_H

#include <vector>
#include "LooperWidget.h"
using namespace std;

#define NUMBER_OF_FSRS 3

class LooperWidget;

class FSR {
public:
    static vector<LooperWidget*> fsrs[NUMBER_OF_FSRS];
    static void clear();
    static void setValues(float* fsrValues);
};


#endif //RTPIANO_FSR_H
