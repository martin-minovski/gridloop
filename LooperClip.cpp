//
// Created by martin on 4/30/18.
//

#include "LooperClip.h"

LooperClip::LooperClip(int channel, bool master, int offset) {
    firstChunk = new LooperChunk();
    lastChunk = firstChunk;
    this->channel = channel;
    this->master = master;
    this->offset = offset;
}
void LooperClip::writeSample(float sample) {
    if (lastChunk->isFull()) {
        auto newChunk = new LooperChunk();
        lastChunk->setNext(newChunk);
        newChunk->setPrev(lastChunk);
        lastChunk = newChunk;
    }
    lastChunk->writeSample(sample);
}
void LooperClip::launch() {
    voices.emplace_back(new LooperVoice(this));
}
LooperChunk* LooperClip::getFirstChunk() {
    return firstChunk;
}
bool LooperClip::isLastChunk(LooperChunk* chunk) {
    return lastChunk == chunk;
}
float LooperClip::renderVoices() {
    float result = 0;
    for (auto voice : voices) {
        result += voice->getNextSample();
    }
    return result;
}
bool LooperClip::isPlaying() {
    for (auto voice : voices) {
        if (!voice->finished()) return true;
    }
    return false;
}
bool LooperClip::isMaster() {
    return master;
}
int LooperClip::getOffset() {
    return offset;
}