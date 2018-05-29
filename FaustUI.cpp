//
// LoopGrid by Martin Minovski, 2018
//

#include "FaustUI.h"
using namespace std;

void FaustUI::setLooperChannel(int ch) {
    looperChannel = ch;
}
std::vector<LooperWidget*>* FaustUI::getWidgets() {
    return &widgets;
}
void FaustUI::initializeNewWidget() {
    newWidget = new LooperWidget(looperChannel);
}

FaustUI::FaustUI() = default;

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
    // Prepare for the next new widget.
    initializeNewWidget();
}

void FaustUI::addButton(const char *label, FAUSTFLOAT *zone) {

}

void FaustUI::addCheckButton(const char *label, FAUSTFLOAT *zone) {

}