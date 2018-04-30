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
    bool master = false;
    int offset = 0;
    int totalSamples = 0;
public:
    LooperClip(int channel, bool master, int offset);
    void writeSample(float sample);
    void launch();
    LooperChunk* getFirstChunk();
    bool isLastChunk(LooperChunk* chunk);
    float renderVoices();
    bool isPlaying();
    bool isMaster();
    int getOffset();
    int getTotalSamples();
};

#endif //RTPIANO_LOOPERCLIP_H
