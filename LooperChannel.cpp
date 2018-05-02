//
// Created by martin on 5/2/18.
//

#include "LooperChannel.h"

LooperChannel::LooperChannel(int id) {
    this->id = id;
}
float LooperChannel::process(float sample) {
    return soloMute ? 0.0f : sample * volume;
}
void LooperChannel::setVolume(float volume) {
    this->volume = volume;
}