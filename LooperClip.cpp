//
// Created by martin on 4/30/18.
//

#include "LooperClip.h"
#include <algorithm>

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
    totalSamples++;
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
        if (voice->finished()) {
            voices.erase(std::remove(voices.begin(), voices.end(), voice), voices.end());
            delete voice;
        }
        else result += voice->getNextSample();
    }
    return result;
}
bool LooperClip::isPlaying() {
    return !voices.empty();
}
bool LooperClip::isMaster() {
    return master;
}
int LooperClip::getOffset() {
    return offset;
}
int LooperClip::getTotalSamples() {
    return totalSamples;
}