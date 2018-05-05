//
// Created by martin on 5/5/18.
//

#include "FaustUI.h"
using namespace std;

FaustUI::FaustUI() {
    newWidget = new LooperWidget();
}

void FaustUI::declare(float * zone, const char * key, const char * val) {
    if (strcmp(key, "axis") == 0) {
        newWidget->setAxis(val);
    }
    else if (strcmp(key, "type") == 0) {
        newWidget->setType(val);
    }
}

void FaustUI::addHorizontalSlider(const char *label,
                                  FAUSTFLOAT *zone,
                                  FAUSTFLOAT init,
                                  FAUSTFLOAT min,
                                  FAUSTFLOAT max,
                                  FAUSTFLOAT step) {
    newWidget->setParams(label, zone, min, max, step);
    widgets.push_back(newWidget);
    newWidget->printData();
    // Prepare for the next new widget.
    newWidget = new LooperWidget();
}

void FaustUI::addButton(const char *label, FAUSTFLOAT *zone) {

}

void FaustUI::addCheckButton(const char *label, FAUSTFLOAT *zone) {

}