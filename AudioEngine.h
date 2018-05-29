//
// LoopGrid by Martin Minovski, 2018
//

#ifndef RTPIANO_AUDIOENGINE_H
#define RTPIANO_AUDIOENGINE_H

#include "rtaudio/RtAudio.h"
#include <map>
#include <stdio.h>

class AudioEngine {
    RtAudio* dac;
public:
    AudioEngine(unsigned int sampleRate, unsigned int bufferFrames, RtAudioCallback render, RtAudio::Api api);
    void probeDevices();
    void printCurrentAudioDriver();
    void shutDown();
};


#endif //RTPIANO_AUDIOENGINE_H
