//
// Created by martin on 5/10/18.
//

#include "PitchDetector.h"

PitchDetector::PitchDetector(unsigned int sampleRate, unsigned int windowSize) {
    aubioWindow = windowSize;
    aubioHopsize = windowSize / 4;
    aubioRate = sampleRate;
    aubioInput = new_fvec(aubioHopsize);
    aubioOutput = new_fvec(1);
    aubioPitch = new_aubio_pitch("default", aubioWindow, aubioHopsize, aubioRate);
}
void PitchDetector::process(float sample) {
    if (sampleCounter >= aubioHopsize) {
        aubio_pitch_do(aubioPitch, aubioInput, aubioOutput);
        currentPitch = fvec_get_sample(aubioOutput, 0);
        sampleCounter = 0;
        fvec_zeros(aubioInput);
        fvec_zeros(aubioOutput);
    }
    fvec_set_sample(aubioInput, sample, sampleCounter);
    sampleCounter++;
}
float PitchDetector::getPitch() {
    return currentPitch;
}