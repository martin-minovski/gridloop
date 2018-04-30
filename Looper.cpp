//
// Created by martin on 4/24/18.
//

#include <iostream>
#include "Looper.h"

using namespace std;

float Looper::process(float sample) {
    if (recordingClip) {
        recordingClip->writeSample(sample);
    }
    float finalSample = 0;
    for (auto clip : clips) {
        schedule(clip);
        finalSample += clip->renderVoices();
    }
    return finalSample;
}
void Looper::startRec() {
    if (recordingClip) return;
    bool isMaster = clips.empty(); // TODO: Review
    recordingClip = new LooperClip(activeChannel, isMaster, isMaster ? 0 : timer);
}
void Looper::stopRec() {
    if (!recordingClip) return;
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
        }
        else {
            timer++;
        }
    }
    else {
        if (!clip->isPlaying() && timer >= clip->getOffset()) {
            clip->launch();
        }
    }
}