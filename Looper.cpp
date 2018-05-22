//
// Created by martin on 4/24/18.
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

                float unconfirmedSample = clip->renderVoices();
                if (clip->getVariation() == 0 || channels[i]->getVariation() == clip->getVariation()) {
                    channelSample += unconfirmedSample;
                }
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
}
void Looper::stopRec() {
    if (!recordingClip) return;
    if (!recordingClip->isMaster()) {
        double ratio = (double)recordingClip->getTotalSamples() / (double)masterClip->getTotalSamples();
        int period = (int)round(ratio);
        if (period < 1) period = 1;
        recordingClip->setSchedulePeriod(period);

        // Catch up on samples to ensure immediate playback in the case of rounding down
        if (ratio - period > 0) {
            int samplesToCatchUp = recordingClip->getTotalSamples() % masterClip->getTotalSamples();
            recordingClip->launch(samplesToCatchUp);
            recordingClip->slaveReschedule();
            recordingClip->slaveScheduleTick();
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
    recordingClip = nullptr;
}
void Looper::setActiveChannel(int channel) {
    activeChannel = channel;
}
void Looper::setActiveVariation(int variation) {
    activeVariation = variation;
    channels[activeChannel]->setVariation(variation);
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
        channels[i]->soloMute = soloEnabled ? !channels[i]->solo : false;
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
    if (clearAll) {
        masterClip = nullptr;
        clipIt = std::begin(clips);
        while (clipIt != std::end(clips)) {
            auto clip = *clipIt;
            clip->purge();
            clipIt = clips.erase(clipIt);
            delete clip;
        }
    }
}