//
// Created by martin on 4/24/18.
//

#include <iostream>
#include "Looper.h"
#include <cmath>

using namespace std;

Looper::Looper() {
    for (int i = 0; i < numChannels; i++) {
        channels[i] = new LooperChannel(i);
    }
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
                channelSample += clip->renderVoices();
            }
        }
        finalSample += channels[i]->process(channelSample);
    }
    return finalSample;
}
void Looper::startRec() {
    if (recordingClip) return;
    bool isMaster = clips.empty();
    recordingClip = new LooperClip(activeChannel, isMaster, isMaster ? 0 : timer);
}
void Looper::stopRec() {
    if (!recordingClip) return;
    if (!recordingClip->isMaster()) {
        double ratio = (double)recordingClip->getTotalSamples() / (double)masterClip->getTotalSamples();
        int period = (int)round(ratio);
        if (period < 1) period = 1;
        recordingClip->setSchedulePeriod(period);
    }
    else masterClip = recordingClip;
    clips.push_back(recordingClip);
    recordingClip = nullptr;
}
void Looper::setActiveChannel(int channel) {
    activeChannel = channel;
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