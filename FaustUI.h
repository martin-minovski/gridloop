//
// LoopGrid by Martin Minovski, 2018
//

#ifndef RTPIANO_FAUSTUI_H
#define RTPIANO_FAUSTUI_H

#include "faust/gui/UI.h"
#include "LooperWidget.h"
#include <vector>

class FaustUI : public UI {
    std::vector<LooperWidget*> widgets;
    LooperWidget* newWidget;
    int looperChannel = 0;

public:

    void setLooperChannel(int ch);
    std::vector<LooperWidget*>* getWidgets();
    void initializeNewWidget();

    FaustUI();

    // Active widgets
    virtual void addButton(const char* label, FAUSTFLOAT* zone);
    virtual void addCheckButton(const char* label, FAUSTFLOAT* zone);
    virtual void addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step);

    // Metadata declarations
    virtual void declare(FAUSTFLOAT* zone, const char* key, const char* value);

    // Not implemented
    virtual void openTabBox(const char* label) {};
    virtual void openHorizontalBox(const char* label) {};
    virtual void openVerticalBox(const char* label) {};
    virtual void closeBox() {};
    virtual void addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT lo, FAUSTFLOAT hi, FAUSTFLOAT step) {};
    virtual void addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT lo, FAUSTFLOAT hi, FAUSTFLOAT step) {};
    virtual void addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT lo, FAUSTFLOAT hi) {};
    virtual void addVerticalBargraph  (const char* label, FAUSTFLOAT* zone, FAUSTFLOAT lo, FAUSTFLOAT hi) {};
    virtual void addSoundfile(const char* label, const char* filename, Soundfile** sf_zone) {};


    void faustNoteOn(int pitch, int velocity);
    void faustNoteOff(int pitch);
    void faustSustain(bool value);
    void FaustCC(int id, int value);

};


#endif //RTPIANO_FAUSTUI_H
