//
// LoopGrid by Martin Minovski, 2018
//

#include "Theremin.h"
using namespace std;

vector<LooperWidget*> Theremin::inputs[NUMBER_OF_INPUTS];

void Theremin::clear() {
    for (int i = 0; i < NUMBER_OF_INPUTS; i++) {
        Theremin::inputs[i].empty();
    }
}

void Theremin::setValues(float* fsrValues) {
    for (int i = 0; i < NUMBER_OF_INPUTS; i++) {
        for (auto widget : Theremin::inputs[i]) {
            // FSRs - apply min / max range of slider
            if (i < 3) widget->setNormalValue(fsrValues[i]);
            // Rest of inputs - apply raw
            else widget->setValue(fsrValues[i]);
        }
    }
}