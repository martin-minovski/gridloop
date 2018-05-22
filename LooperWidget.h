//
// Created by martin on 5/5/18.
//

#ifndef RTPIANO_LOOPERWIDGET_H
#define RTPIANO_LOOPERWIDGET_H

#include <cstring>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include "json.hpp"
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

public:
    LooperWidget(int channel);
    void setParams(const char* name, float* zone, float min, float max, float step);
    void setType(const char* type);
    void setAxis(const char* axis);
    void printData();
    json getJson();
    bool isSync();
    void setValue(float value);
};


#endif //RTPIANO_LOOPERWIDGET_H
