//
// Created by martin on 4/30/18.
//

#include "LooperVoice.h"

LooperVoice::LooperVoice(LooperClip* clip) {
    vClip = clip;
    vChunk = clip->getFirstChunk();
    vSample = 0;
}
bool LooperVoice::finished() {
    if (vClip->isLastChunk(vChunk)) {
        if (vSample >= vChunk->getWriter()) {
            return true;
        }
    }
    if (!vChunk) return true; // Bugfix. No idea
    return false;
}
float LooperVoice::getNextSample() {
    if (finished()) {
        return 0;
    }
    float sample = vChunk->getSample(vSample++);
    if (vSample >= vChunk->getSize()) {
        vSample = 0;
        vChunk = vChunk->getNext();
    }
    return sample;
}
void LooperVoice::fastForward(int samples) {
    for (int i = 0; i < samples; i++) {
        getNextSample();
    }
}