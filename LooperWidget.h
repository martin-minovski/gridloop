//
// Created by martin on 5/5/18.
//

#ifndef RTPIANO_LOOPERWIDGET_H
#define RTPIANO_LOOPERWIDGET_H

#include <cstring>
#include <cassert>
#include <cstdlib>
#include <iostream>
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

public:
    LooperWidget();
    void setParams(const char* name, float* zone, float min, float max, float step);
    void setType(const char* type);
    void setAxis(const char* axis);
    float* getZone();
    void printData();
};


#endif //RTPIANO_LOOPERWIDGET_H
