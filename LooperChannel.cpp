//
// Created by martin on 5/2/18.
//

#include "LooperChannel.h"
#include <iostream>
using namespace std;

LooperChannel::LooperChannel(int id) {
    this->id = id;

    string error_msg;
    char filepath[16];
//    snprintf(filepath, sizeof filepath, "%s%d%s", "dsp/", i, ".dsp");
    snprintf(filepath, sizeof filepath, "%s", "dsp/1.dsp");
    faustFactory = createDSPFactoryFromFile(filepath, 0, 0, "", error_msg);
    cout << error_msg;
    faustDSP = faustFactory->createDSPInstance();
    faustDSP->init(44100);
    faustDSP->buildUserInterface(&faustUI);
}
float LooperChannel::process(float sample) {
    float input = soloMute ? 0 : sample * volume;
    float output = 0;
    if (leftChannel) {
        faustInL[0] = input;
        output = faustOutL[0];
    }
    else {
        faustInR[0] = input;
        output = faustOutR[0];
        faustDSP->compute(bufferSize, faustIn, faustOut);
    }
    leftChannel = !leftChannel;

    return output;
}
void LooperChannel::setVolume(float volume) {
    this->volume = volume;
}