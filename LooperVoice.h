//
// LoopGrid by Martin Minovski, 2018
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
    void fastForward(int samples);
    void runWidgetAutomation();
};


#endif //RTPIANO_LOOPERVOICE_H
