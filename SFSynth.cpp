//
// Created by martin on 4/28/18.
//

#include "SFSynth.h"

#define TSF_IMPLEMENTATION

#include "tsf.h"

tsf* tsfObj(long long ptr) {
    return (tsf*)ptr;
}
SFSynth::SFSynth(unsigned int sampleRate, unsigned int bufferSize) {
    this->bufferSize = bufferSize;
    tsfPtr = (long long)tsf_load_filename("omega.sf2");
    tsf_set_output(tsfObj(tsfPtr), TSF_STEREO_UNWEAVED, sampleRate, 0);
    for (int i = 0; i < 128; i++) {
        sustained[i] = 0;
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
void SFSynth::setGain(float gain) {
    this->gain = gain;
}
float SFSynth::getNextSample() {
    if (bufferCounter >= bufferSize) {
        // TODO: Multithreading?
        tsf_render_float(tsfObj(tsfPtr), buffer, bufferSize, 0);
        bufferCounter = 0;
    }
    return buffer[bufferCounter++] * gain;
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