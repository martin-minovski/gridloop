//
// Created by martin on 4/28/18.
//


#include "SFSynth.h"
#define TSF_IMPLEMENTATION
#include "tsf.h"
#include <mutex>

//using namespace std::chrono_literals;

long long SFSynth::tsfPtr;
float SFSynth::bufferA[4096];
float SFSynth::bufferB[4096];
bool SFSynth::writingA;
int SFSynth::bufferSize;
int SFSynth::bufferCounter;
int SFSynth::sustained[127];
bool SFSynth::isSustaining;
int SFSynth::tsfPreset;
float SFSynth::gain;
int SFSynth::presetCount;
thread SFSynth::workerThread;
std::future<void> SFSynth::future;

static tsf* tsfObj(long long ptr) {
    return (tsf*)ptr;
}
void SFSynth::init(unsigned int sampleRate, unsigned int _bufferSize) {
    isSustaining = false;
    gain = 0.2;
    presetCount = 0;
    tsfPreset = 0;
    bufferSize = _bufferSize;
    bufferCounter = bufferSize;
    writingA = true;
    tsfPtr = (long long)tsf_load_filename("nicekeys.sf2");
    tsf_set_output(tsfObj(tsfPtr), TSF_STEREO_UNWEAVED, sampleRate, 0);
    for (int i = 0; i < 128; i++) {
        sustained[i] = 0;
    }
    for (int i = 0; i < bufferSize; i++) {
        bufferA[i] = 0;
        bufferB[i] = 0;
    }
}
void SFSynth::setPreset(int bank, int preset) {
    panic();
    tsfPreset = tsf_get_presetindex(tsfObj(tsfPtr), bank, preset);
}
void SFSynth::noteOn(int pitch, int velocity) {
    tsf_note_off(tsfObj(tsfPtr), tsfPreset, pitch);
    tsf_note_on(tsfObj(tsfPtr), tsfPreset, pitch, (velocity+1)/128.0f);
    sustained[pitch] = false;
}
void SFSynth::noteOff(int pitch) {
    if (isSustaining) sustained[pitch] = 1;
    else tsf_note_off(tsfObj(tsfPtr), tsfPreset, pitch);
}
void SFSynth::setGain(float _gain) {
    gain = _gain;
}

void SFSynth::renderWorker() {
}

std::mutex mx;

float SFSynth::getNextSample() {
    if (bufferCounter >= bufferSize) {
        bufferCounter = 0;
        writingA = !writingA;
        future = std::async(std::launch::async, [] {
            mx.lock();
            float* writing = bufferB;
            if (writingA) writing = bufferA;
            tsf_render_float((tsf*)tsfPtr, writing, bufferSize, 0);
            mx.unlock();
        });
    }
    float* reading = bufferA;
    if (writingA) reading = bufferB;
    float result = reading[bufferCounter] * gain;
    bufferCounter++;
    return result;
}
void SFSynth::sustainOff() {
    isSustaining = false;
    for (int i = 0; i < 128; i++) {
        if (sustained[i]) tsf_note_off(tsfObj(tsfPtr), tsfPreset, i);
    }
}
void SFSynth::sustainOn() {
    isSustaining = true;
}
void SFSynth::panic() {
    isSustaining = false;
    for (int i = 0; i < 128; i++) {
        sustained[i] = 0;
        noteOff(i);
    }
}

json SFSynth::getInstruments() {
    presetCount = tsf_get_presetcount(tsfObj(tsfPtr));
    json result = {};
    for (int i = 0; i < 128; i++) {
        json bank = {};
        bool empty = true;
        for (int j = 0; j < 128; j++) {
            const char* name = tsf_bank_get_presetname(tsfObj(tsfPtr), i, j);
            if (name != nullptr) {
                bank[to_string(j)] = string(name);
                empty = false;
            }
        }
        if (!empty) result[to_string(i)] = bank;
    }
    return result;
}

