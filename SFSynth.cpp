//
// LoopGrid by Martin Minovski, 2018
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
int SFSynth::tsfDrumPreset = 0;
float SFSynth::gain;
int SFSynth::presetCount;
thread SFSynth::workerThread;
std::future<void> SFSynth::future;
int SFSynth::currentPreset = 0;
int SFSynth::currentBank = 0;

void SFSynth::nextInstrument() {
    SFSynth::currentPreset++;
    SFSynth::setPreset(0, ++(SFSynth::currentPreset));
}
void SFSynth::prevInstrument() {
    if (SFSynth::currentPreset > 0) SFSynth::setPreset(0, --(SFSynth::currentPreset));
}
tsf* tsfObj(long long ptr) {
    return (tsf*)ptr;
}
void SFSynth::setPitchWheel(int value) {
    tsf_channel_set_pitchwheel(tsfObj(tsfPtr), 0, value);
}
void SFSynth::init(unsigned int sampleRate, unsigned int _bufferSize) {
    isSustaining = false;
    gain = 0.2;
    presetCount = 0;
    tsfPreset = 0;
    bufferSize = _bufferSize;
    bufferCounter = bufferSize;
    writingA = true;
    tsfPtr = (long long)tsf_load_filename("sf2/nicekeys.sf2");
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
    if (getInstruments()[std::to_string(bank)] != NULL && getInstruments()[std::to_string(bank)][std::to_string(preset)] != NULL) {
        if (bank == 127) {
            tsfDrumPreset = tsf_get_presetindex(tsfObj(tsfPtr), bank, preset);
        }
        else {
            tsfPreset = tsf_get_presetindex(tsfObj(tsfPtr), bank, preset);
        }
        cout << "B" << bank << " P" << preset << " - " << tsf_bank_get_presetname(tsfObj(tsfPtr), bank, preset) << endl;
    }
}
void SFSynth::noteOn(int pitch, int velocity) {
    tsf_note_off(tsfObj(tsfPtr), tsfPreset, pitch);
    tsf_note_on(tsfObj(tsfPtr), tsfPreset, pitch, (velocity+1)/128.0f);
    sustained[pitch] = false;
}
void SFSynth::drumOn(int pitch, int velocity) {
    tsf_note_off(tsfObj(tsfPtr), tsfDrumPreset, pitch);
    tsf_note_on(tsfObj(tsfPtr), tsfDrumPreset, pitch, (velocity+1)/128.0f);
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
            else {
                bank[to_string(j)] = NULL;
            }
        }
        if (!empty) result[to_string(i)] = bank;
        else result[to_string(i)] = NULL;
    }
    return result;
}

