//
// Created by martin on 4/24/18.
//

#ifndef RTPIANO_LOOPER_H
#define RTPIANO_LOOPER_H

#include <vector>
#include "LooperClip.h"
#include "LooperChannel.h"
using namespace std;

#define NUMBER_OF_LOOPER_CHANNELS 3

class Looper {
    LooperClip* recordingClip = nullptr;
    LooperClip* masterClip = nullptr;
    vector<LooperClip*> clips;
    int timer = 0;
    int activeChannel = 0;
    LooperChannel* channels[NUMBER_OF_LOOPER_CHANNELS];
    int numChannels = NUMBER_OF_LOOPER_CHANNELS;
public:
    Looper();
    float process(float sample);
    void startRec();
    void stopRec();
    void setActiveChannel(int channel);
    void schedule(LooperClip* clip);
    void setChannelSolo(int ch, bool solo);
    void setChannelVolume(int ch, float volume);
};


#endif //RTPIANO_LOOPER_H
