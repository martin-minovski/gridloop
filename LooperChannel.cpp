//
// Created by martin on 5/2/18.
//

#include "LooperChannel.h"
#include <iostream>
using namespace std;



LooperChannel::LooperChannel(int id, OSC* osc) {
    this->id = id;
    this->osc = osc;
    faustDSP = nullptr;
    reloadDSPFile();
}
bool LooperChannel::reloadDSPFile() {
    // First clear Theremin registers
    Theremin::clear();

    faustUI = FaustUI();
    faustUI.setLooperChannel(id);
    faustUI.initializeNewWidget();
    llvm_dsp_factory* newFaustFactory;
    dsp* newFaustDSP;
    string error_msg;
    char filepath[16];
    snprintf(filepath, sizeof filepath, "%s%d%s", "dsp/", id, ".dsp");
    newFaustFactory = createDSPFactoryFromFile(filepath, 0, 0, "", error_msg);
    if (error_msg.length() > 0) {
        string final_error = "Channel " + error_msg;
        osc->sendFaustError(final_error.c_str());
        return false;
    }
    newFaustDSP = newFaustFactory->createDSPInstance();
    newFaustDSP->init(44100);
    newFaustDSP->buildUserInterface(&faustUI);
    faustDSP = newFaustDSP;
    return true;
}
float LooperChannel::process(float sample) {
    float output = 0;

    if (leftChannel) {
        faustInL[0] = sample;
        output = faustOutL[0] * volume;
    }
    else {
        faustInR[0] = sample;
        output = faustOutR[0] * volume;
        if (faustDSP != nullptr) faustDSP->compute(bufferSize, faustIn, faustOut);
    }
    leftChannel = !leftChannel;


    // Prevent clicking noise when (un)muting / (un)soloing channels

    if (!muteFinal && graceRoundCountdown) {
        output *= ((float)(graceRoundCeil - graceRoundCountdown) / graceRoundCeil);
        graceRoundCountdown--;
    }
    else if (muteFinal && graceRoundCountdown) {
        output *= ((float)graceRoundCountdown / graceRoundCeil);
        graceRoundCountdown--;
    }
    else if (muteFinal) {
        output = 0;
    }

    return output;
}
void LooperChannel::setVolume(float volume) {
    this->volume = volume;
}
std::vector<LooperWidget*>* LooperChannel::getWidgets() {
    return faustUI.getWidgets();
}
void LooperChannel::setVariation(int variation) {
    this->variation = variation;
}
int LooperChannel::getVariation() {
    return variation;
}
float LooperChannel::getVolume() {
    return volume;
}
void LooperChannel::setSoloMute(bool soloMute) {
    this->soloMute = soloMute;
    setFinalMuteSoft(soloMute || muteMute);
}
void LooperChannel::setMute(bool mute) {
    this->muteMute = mute;
    setFinalMuteSoft(soloMute || muteMute);
}
void LooperChannel::setFinalMuteSoft(bool mute) {
    if (mute != muteFinal) {
        if (graceRoundCountdown) graceRoundCountdown = graceRoundCeil - graceRoundCountdown;
        else graceRoundCountdown = graceRoundCeil;
        this->muteFinal = mute;
    }
}