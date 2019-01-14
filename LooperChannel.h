//
// LoopGrid by Martin Minovski, 2018
//

#ifndef RTPIANO_LOOPERCHANNEL_H
#define RTPIANO_LOOPERCHANNEL_H

#include "faust/dsp/dsp.h"
#include "faust/dsp/llvm-dsp.h"
#include "faust/gui/UI.h"
#include "FaustUI.h"
#include "OSC.h"
#include "Theremin.h"

#define FAUST_BUFFER_SIZE 32

class LooperChannel {
    int id;
    int variation = 0;
    float volume = 0.5f;
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
    int bufferCounter = 0;
    float leftChannel = true;
    float* faustIn[2] = {faustInL, faustInR};
    float* faustOut[2] = {faustOutL, faustOutR};

    bool soloMute = false;
    bool muteMute = false;
    bool muteFinal = false;
    void setFinalMuteSoft(bool mute);
    int graceRoundCountdown = 0;
    int graceRoundCeil = 2048;
public:
    bool solo = false;
    LooperChannel(int id, OSC* osc);
    float process(float sample);
    void setVolume(float volume);
    std::vector<LooperWidget*>* getWidgets();
    bool reloadDSPFile();
    void setVariation(int variation);
    int getVariation();
    float getVolume();
    void setSoloMute(bool soloMute);
    void setMute(bool mute);
    void faustNoteOn(int pitch, int velocity);
    void faustNoteOff(int pitch);
    void faustSustain(bool value);
    std::vector<long> FaustCC(int id, int value, int channel);
};


#endif //RTPIANO_LOOPERCHANNEL_H
