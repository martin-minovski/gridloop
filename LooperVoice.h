//
// Created by martin on 4/30/18.
//

#ifndef RTPIANO_LOOPERVOICE_H
#define RTPIANO_LOOPERVOICE_H

#include "LooperChunk.h"
#include "LooperClip.h"

class LooperClip;
class LooperChunk;
class LooperVoice {
    int vSample;
    LooperChunk* vChunk;
    LooperClip* vClip;
public:
    LooperVoice(LooperClip *clip);
    bool finished();
    float getNextSample();
};


#endif //RTPIANO_LOOPERVOICE_H
