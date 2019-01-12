//
// LoopGrid by Martin Minovski, 2018
//

#ifndef RTPIANO_LOOPERWIDGET_H
#define RTPIANO_LOOPERWIDGET_H

#include <cstring>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include "json.hpp"
#include "Theremin.h"
using json = nlohmann::json;

using namespace std;

class LooperWidget {
    char* name;

    float* zone;
    float min;
    float max;
    float step;

    // NexusUI params
    int axis = 0;
    char* type;
    int looperChannel = 0;

    // MIDI CC key
    int id = -1;

public:
    LooperWidget(int channel);
    void setParams(const char* name, float* zone, float min, float max, float step);
    char* getType();
    void setType(const char* type);
    void setAxis(const char* axis);
    void printData();
    json getJson();
    bool isSync();
    void setValue(float value);
    void setNormalValue(float value);
    void setId(int id);
    int getId();
};


#endif //RTPIANO_LOOPERWIDGET_H
