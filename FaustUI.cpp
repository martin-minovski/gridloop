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
    else if (strcmp(key, "id") == 0) {
        newWidget->setId(atoi(val));
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
void FaustUI::faustNoteOn(int pitch, int velocity) {
    // First set pitch and velocity
    auto widgetIt = std::begin(widgets);
    while (widgetIt != std::end(widgets)) {
        auto widget = *widgetIt;
        if (strcmp(widget->getType(), "midipitch") == 0) {
            widget->setValue(pitch);
        }
        else if (strcmp(widget->getType(), "midivelocity") == 0) {
            widget->setValue(velocity);
        }
        ++widgetIt;
    }

    // Fire the note
    widgetIt = std::begin(widgets);
    while (widgetIt != std::end(widgets)) {
        auto widget = *widgetIt;
        if (strcmp(widget->getType(), "midinoteon") == 0) {
            widget->setValue(1);
        }
        else if (strcmp(widget->getType(), "midinoteoff") == 0) {
            widget->setValue(0);
        }
        ++widgetIt;
    }
}
void FaustUI::faustNoteOff(int pitch) {
    // First set pitch and velocity
    auto widgetIt = std::begin(widgets);
    while (widgetIt != std::end(widgets)) {
        auto widget = *widgetIt;
        if (strcmp(widget->getType(), "midipitch") == 0) {
            widget->setValue(pitch);
        }
        ++widgetIt;
    }

    // Fire the note
    widgetIt = std::begin(widgets);
    while (widgetIt != std::end(widgets)) {
        auto widget = *widgetIt;
        if (strcmp(widget->getType(), "midinoteon") == 0) {
            widget->setValue(0);
        }
        else if (strcmp(widget->getType(), "midinoteoff") == 0) {
            widget->setValue(1);
        }
        ++widgetIt;
    }
}
void FaustUI::faustSustain(bool value) {
    auto widgetIt = std::begin(widgets);
    while (widgetIt != std::end(widgets)) {
        auto widget = *widgetIt;
        if (strcmp(widget->getType(), "midisustain") == 0) {
            widget->setValue((int)value);
        }
        ++widgetIt;
    }
}
void FaustUI::FaustCC(int id, int value) {
    auto widgetIt = std::begin(widgets);
    while (widgetIt != std::end(widgets)) {
        auto widget = *widgetIt;
        if (strcmp(widget->getType(), "midicc") == 0 && id == widget->getId()) {
            widget->setNormalValue(((float)value)/127);
        }
        ++widgetIt;
    }
}