//
// Created by martin on 5/5/18.
//

#include "FSR.h"
using namespace std;

vector<LooperWidget*> FSR::fsrs[NUMBER_OF_FSRS];

void FSR::clear() {
    for (int i = 0; i < NUMBER_OF_FSRS; i++) {
        FSR::fsrs[i].empty();
    }
}

void FSR::setValues(float* fsrValues) {
    for (int i = 0; i < NUMBER_OF_FSRS; i++) {
        for (auto widget : FSR::fsrs[i]) {
            widget->setNormalValue(fsrValues[i]);
        }
    }
}