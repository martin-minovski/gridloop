//
// Created by martin on 4/24/18.
//

#ifndef RTPIANO_LOOPER_H
#define RTPIANO_LOOPER_H

#include <vector>
#include "LooperClip.h"
#include "LooperChannel.h"
#include <string>
#include "OSC.h"
#include "json.hpp"
using json = nlohmann::json;
using namespace std;

#define NUMBER_OF_LOOPER_CHANNELS 8

class Looper {
    LooperClip* recordingClip = nullptr;
    LooperClip* masterClip = nullptr;
    vector<LooperClip*> clips;
    int timer = 0;
    int activeChannel = 0;
    int activeVariation = 0;
    LooperChannel* channels[NUMBER_OF_LOOPER_CHANNELS];
    int numChannels = NUMBER_OF_LOOPER_CHANNELS;
    unsigned int sampleRate;
public:
    Looper(OSC* osc, unsigned int sampleRate);
    float process(float sample);
    void startRec();
    void stopRec();
    void setActiveChannel(int channel);
    void setActiveVariation(int variation);
    void schedule(LooperClip* clip);
    void setChannelSolo(int ch, bool solo);
    void setChannelVolume(int ch, float volume);
    string getWidgetJSON();
    bool reloadChannelDSP(int channel);
    int getActiveChannel();
    int getActiveVariation();
    json getClipSummary();
    json getChannelSummary();
    void clearChannel(int chNum, int varNum);
    void storeWidgetAutomation(long pointer, float value);
    bool isRecording();
    void setAllVariations(int variation);
};


#endif //RTPIANO_LOOPER_H
