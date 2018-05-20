//
// Created by martin on 5/2/18.
//

#ifndef RTPIANO_LOOPERCHANNEL_H
#define RTPIANO_LOOPERCHANNEL_H

#include "faust/dsp/dsp.h"
#include "faust/dsp/llvm-dsp.h"
#include "faust/gui/UI.h"
#include "FaustUI.h"
#include "OSC.h"

#define FAUST_BUFFER_SIZE 1

class LooperChannel {
    int id;
    int variation = 0;
    float volume = 1.0f;
    OSC* osc;
    // Faust
    llvm_dsp_factory* faustFactory;
    dsp* faustDSP;
    FaustUI faustUI;
    float faustInL[FAUST_BUFFER_SIZE];
    float faustInR[FAUST_BUFFER_SIZE];
    float faustOutL[FAUST_BUFFER_SIZE];
    float faustOutR[FAUST_BUFFER_SIZE];
    int bufferSize = FAUST_BUFFER_SIZE;
    float leftChannel = true;
    float* faustIn[2] = {faustInL, faustInR};
    float* faustOut[2] = {faustOutL, faustOutR};
public:
    bool solo = false;
    bool soloMute = false;
    LooperChannel(int id, OSC* osc);
    float process(float sample);
    void setVolume(float volume);
    std::vector<LooperWidget*>* getWidgets();
    bool reloadDSPFile();
    void setVariation(int variation);
    int getVariation();
    float getVolume();
};


#endif //RTPIANO_LOOPERCHANNEL_H
