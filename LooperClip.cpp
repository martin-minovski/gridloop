//
// Created by martin on 4/30/18.
//

#include "LooperClip.h"
#include <algorithm>

LooperClip::LooperClip(int channel, int variation, bool master, int offset) {
    firstChunk = new LooperChunk();
    lastChunk = firstChunk;
    this->channel = channel;
    this->variation = variation;
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
void LooperClip::launch(int fastForward) {
    auto voice = new LooperVoice(this);
    voice->fastForward(fastForward);
    voices.emplace_back(voice);
}
LooperChunk* LooperClip::getFirstChunk() {
    return firstChunk;
}
bool LooperClip::isLastChunk(LooperChunk* chunk) {
    return lastChunk == chunk;
}
float LooperClip::renderVoices(bool unmute) {
    float result = 0;
    auto voiceIt = std::begin(voices);
    while (voiceIt != std::end(voices)) {
        auto voice = *voiceIt;
        if (voice->finished()) {
            voiceIt = voices.erase(voiceIt);
            delete voice;
        }
        else {
            voice->runWidgetAutomation();
            result += voice->getNextSample();
            ++voiceIt;
        }
    }

    // Prevent clicking noise when switching between variations

    setUnmuteSoft(unmute);

    if (unmute && graceRoundCountdown) {
        result *= ((float)(graceRoundCeil - graceRoundCountdown) / graceRoundCeil);
        graceRoundCountdown--;
    }
    else if (!unmute && graceRoundCountdown) {
        result *= ((float)graceRoundCountdown / graceRoundCeil);
        graceRoundCountdown--;
    }
    else if (!unmute) {
        result = 0;
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
void LooperClip::slaveScheduleTick() {
    scheduleCountdown--;
}
bool LooperClip::slaveScheduled() {
    return scheduleCountdown <= 0;
}
void LooperClip::slaveReschedule() {
    scheduleCountdown = schedulePeriod;
}
void LooperClip::setSchedulePeriod(int period) {
    schedulePeriod = period;
}
int LooperClip::getChannel() {
    return channel;
}
int LooperClip::getVariation() {
    return variation;
}
void LooperClip::setVariation(int value) {
    variation = value;
}
void LooperClip::purge() {
    auto voiceIt = std::begin(voices);
    while (voiceIt != std::end(voices)) {
        auto voice = *voiceIt;
        voiceIt = voices.erase(voiceIt);
        delete voice;
    }
}
void LooperClip::storeWidgetAutomation(long pointer, float value) {
    lastChunk->writeZone(pointer, value);
}
void LooperClip::roundOut() {
    lastChunk->roundOut();
}
void LooperClip::roundIn() {
    firstChunk->roundIn();
}
void LooperClip::recorderNotify() {
    recorderNotifications++;
}
int LooperClip::getRecorderNotifications() {
    return recorderNotifications;
}
void LooperClip::setUnmuteHard(bool unmute) {
    this->unmute = unmute;
}
void LooperClip::setUnmuteSoft(bool unmute) {
    if (unmute != this->unmute) {
        if (graceRoundCountdown) graceRoundCountdown = graceRoundCeil - graceRoundCountdown;
        else graceRoundCountdown = graceRoundCeil;
        this->unmute = unmute;
    }
}