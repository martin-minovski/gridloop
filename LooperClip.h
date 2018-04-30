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
    int channel;
    bool master;
    int offset;
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
};

#endif //RTPIANO_LOOPERCLIP_H
