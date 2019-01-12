//
// LoopGrid by Martin Minovski, 2018
//

#ifndef RTPIANO_SFSYNTH_H

#define RTPIANO_SFSYNTH_H


#include <cstring>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <thread>
#include <chrono>
#include <iostream>
#include <future>
#include "json.hpp"
using json = nlohmann::json;
using namespace std;

class SFSynth {
public:
    static long long tsfPtr;
    static float bufferA[4096];
    static float bufferB[4096];
    static bool writingA;
    static int bufferSize;
    static int bufferCounter;
    static int sustained[127];
    static bool isSustaining;
    static int tsfPreset;
    static float gain;
    static int presetCount;
    static thread workerThread;
    static std::future<void> future;
    static int tsfDrumPreset;
    static int currentPreset;
    static int currentBank;

    static void init(unsigned int sampleRate, unsigned int _bufferSize);
    static void setPreset(int bank, int preset);
    static void noteOn(int pitch, int velocity);
    static void drumOn(int pitch, int velocity);
    static void noteOff(int pitch);
    static void sustainOn();
    static void sustainOff();
    static void setGain(float _gain);
    static float getNextSample();
    static void panic();
    static json getInstruments();
    static void renderWorker();
    static void nextInstrument();
    static void prevInstrument();
};

#endif //RTPIANO_SFSYNTH_H
