//
// LoopGrid by Martin Minovski, 2018
//

#include <iostream>
#include "Looper.h"
#include <cmath>
#include "LooperWidget.h"
#include "json.hpp"
using json = nlohmann::json;

using namespace std;

Looper::Looper(OSC* osc, unsigned int sampleRate) {
    for (int i = 0; i < numChannels; i++) {
        channels[i] = new LooperChannel(i, osc);
    }
    this->sampleRate = sampleRate;
    this->osc = osc;
}
float Looper::process(float sample) {
    if (recordingClip) {
        recordingClip->writeSample(sample);
    }
    float finalSample = 0;

    for (int i = 0; i < numChannels; i++) {
        float channelSample = 0;
        for (auto clip : clips) {
            if (i == clip->getChannel()) {
                schedule(clip);
                bool unmutedVariation =
                        clip->getVariation() == 0 ||
                        channels[i]->getVariation() == clip->getVariation();
                channelSample += clip->renderVoices(unmutedVariation);
            }
        }
        float liveSample = 0;
        if (i == activeChannel) liveSample = sample;
        finalSample += channels[i]->process(channelSample + liveSample);
    }
    return finalSample;
}
void Looper::startRec() {
    if (recordingClip) return;
    bool isMaster = clips.empty();
    recordingClip = new LooperClip(activeChannel, activeVariation, isMaster, isMaster ? 0 : timer);
    osc->sendRecUpdate(true);
}
void Looper::stopRec() {
    if (!recordingClip) return;
    if (!recordingClip->isMaster()) {
        // Sanity check
        if (!masterClip) {
            recordingClip = nullptr;
            return;
        }

        double ratio = (double)recordingClip->getTotalSamples() / (double)masterClip->getTotalSamples();
        int period = (int)round(ratio);
        if (period < 1) period = 1;
        recordingClip->setSchedulePeriod(period);

        // Catch up on samples to ensure immediate playback in the case of rounding down
        if (ratio - period > 0) {
            int samplesToCatchUp = recordingClip->getTotalSamples() % masterClip->getTotalSamples();
            recordingClip->launch(samplesToCatchUp);
            recordingClip->slaveReschedule();
            if (recordingClip->getRecorderNotifications() > period) {
                recordingClip->slaveScheduleTick();
            }
            // Also prevent clicking noise
            recordingClip->setUnmuteHard(false);
        }
    }
    else {
        masterClip = recordingClip;

        // Update all sync'd widgets
        int length = masterClip->getTotalSamples();
        for (int i = 0; i < numChannels; i++) {
            std::vector<LooperWidget*>* widgets = channels[i]->getWidgets();
            for (auto widget : (*widgets)) {
                if (widget->isSync()) {
                    widget->setValue(length / 2); // Divide by 2 channels
                }
            }
        }
    }
    clips.push_back(recordingClip);
    recordingClip->roundIn();
    recordingClip->roundOut();
    recordingClip = nullptr;
    osc->sendRecUpdate(false);
}
void Looper::setActiveChannel(int channel) {
    activeChannel = channel;
}
void Looper::setActiveVariation(int variation) {
    activeVariation = variation;
}
void Looper::setChannelVariation(int channel, int variation) {
    channels[channel]->setVariation(variation);
}
int Looper::getChannelVariation(int channel) {
    return channels[channel]->getVariation();
}
int Looper::getActiveChannel() {
    return activeChannel;
}
int Looper::getActiveVariation() {
    return activeVariation;
}
void Looper::schedule(LooperClip* clip) {
    if (clip->isMaster()) {
        if (!clip->isPlaying()) {
            clip->launch();
            timer = 0;
            for (auto slaveClip : clips) {
                if (!slaveClip->isMaster()) {
                    slaveClip->slaveScheduleTick();
                }
            }
            if (recordingClip) {
                recordingClip->recorderNotify();
            }
        }
        else {
            timer++;
        }
    }
    else {
        if (clip->slaveScheduled() && timer == clip->getOffset()) {
            clip->launch();
            clip->slaveReschedule();
        }
    }
}
void Looper::setChannelSolo(int ch, bool solo) {
    channels[ch]->solo = solo;
    bool soloEnabled = false;
    for (int i = 0; i < numChannels; i++) {
        if (channels[i]->solo) soloEnabled = true;
    }
    for (int i = 0; i < numChannels; i++) {
        bool soloMute = soloEnabled ? !channels[i]->solo : false;
        channels[i]->setSoloMute(soloMute);
    }
}
void Looper::setChannelVolume(int ch, float volume) {
    channels[ch]->setVolume(volume);
}
string Looper::getWidgetJSON() {
    json result;
    for (int i = 0; i < numChannels; i++) {
        std::vector<LooperWidget*>* widgets = channels[i]->getWidgets();
        for (auto widget : (*widgets)) {
//            if (widget->isSync()) continue; // Don't include sync widgets
            result.push_back(widget->getJson());
        }
    }
    return result.dump();
}
bool Looper::reloadChannelDSP(int channel) {
    return channels[channel]->reloadDSPFile();
}
json Looper::getClipSummary() {
    json result;
    for (auto clip : clips) {
        json clipJson;
        clipJson["length"] = clip->getTotalSamples();
        clipJson["channel"] = clip->getChannel();
        clipJson["variation"] = clip->getVariation();
        clipJson["master"] = clip->isMaster();
        result.push_back(clipJson);
    }
    return result;
}
json Looper::getChannelSummary() {
    json result;
    for (int i = 0; i < numChannels; i++) {
        json channelJson;
        channelJson["variation"] = channels[i]->getVariation();
        channelJson["volume"] = channels[i]->getVolume();
        channelJson["solo"] = channels[i]->solo;
        result.push_back(channelJson);
    }
    return result;
}
void Looper::clearChannel(int chNum, int varNum) {
    bool clearAll = false;
    auto clipIt = std::begin(clips);
    while (clipIt != std::end(clips)) {
        auto clip = *clipIt;
        if (clip->getChannel() == chNum && clip->getVariation() == varNum) {
            if (clip->isMaster()) {
                clearAll = true;
                break;
            }
            clip->purge();
            clipIt = clips.erase(clipIt);
            delete clip;
        }
        else ++clipIt;
    }
    if (clearAll) this->clearAll();
}
void Looper::clearAll() {
    masterClip = nullptr;
    auto clipIt = std::begin(clips);
    while (clipIt != std::end(clips)) {
        auto clip = *clipIt;
        clip->purge();
        clipIt = clips.erase(clipIt);
        delete clip;
    }
}
void Looper::undo() {
    auto clip = clips.back();
    if (!clip) return;
    if (clip->isMaster()) masterClip = nullptr;
    clip->purge();
    clips.pop_back();
    delete clip;
}
void Looper::storeWidgetAutomation(long pointer, float value) {
    if (recordingClip) {
        recordingClip->storeWidgetAutomation(pointer, value);
    }
}
bool Looper::isRecording() {
    return recordingClip != nullptr;
}
void Looper::setAllVariations(int variation) {
    for (int i = 0; i < numChannels; i++) {
        channels[i]->setVariation(variation);
    }
}
bool Looper::isSlotEmpty(int channel, int variation) {
    bool empty = true;
    for (auto clip : clips) {
        if (clip->getChannel() == channel && clip->getVariation() == variation) empty = false;
    }
    return empty;
}
void Looper::faustNoteOn(int pitch, int velocity) {
    for (int i = 0; i < numChannels; i++) {
        channels[i]->faustNoteOn(pitch, velocity);
    }
}
void Looper::faustNoteOff(int pitch) {
    for (int i = 0; i < numChannels; i++) {
        channels[i]->faustNoteOff(pitch);
    }
}
void Looper::faustSustain(bool value) {
    for (int i = 0; i < numChannels; i++) {
        channels[i]->faustSustain(value);
    }
}
void Looper::FaustCC(int id, int value) {
    channels[activeChannel]->FaustCC(id, value);
}