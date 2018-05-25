//
// Created by martin on 4/30/18.
//

#ifndef RTPIANO_LOOPERCLIP_H
#define RTPIANO_LOOPERCLIP_H

#include <vector>
#include "LooperChunk.h"
#include "LooperVoice.h"

using namespace std;

class LooperVoice;
class LooperChunk;
class LooperClip {
    LooperChunk* firstChunk;
    LooperChunk* lastChunk;
    vector<LooperVoice*> voices;
    int channel = 0;
    int variation = 0;
    bool master = false;
    int offset = 0;
    int totalSamples = 0;
    int schedulePeriod = 0;
    int scheduleCountdown = 0;
public:
    LooperClip(int channel, int variation, bool master, int offset);
    void writeSample(float sample);
    void launch();
    void launch(int fastForward);
    LooperChunk* getFirstChunk();
    bool isLastChunk(LooperChunk* chunk);
    float renderVoices();
    bool isPlaying();
    bool isMaster();
    int getOffset();
    int getTotalSamples();
    // Scheduler methods
    void setSchedulePeriod(int period);
    void slaveScheduleTick();
    bool slaveScheduled();
    void slaveReschedule();
    int getChannel();
    int getVariation();
    void setVariation(int value);
    void purge();
    void storeWidgetAutomation(long pointer, float value);
    void roundOut();
    void roundIn();
};

#endif //RTPIANO_LOOPERCLIP_H
