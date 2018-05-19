//
// Created by martin on 5/10/18.
//

#ifndef RTPIANO_PITCHDETECTOR_H
#define RTPIANO_PITCHDETECTOR_H

#include "aubio/aubio.h"
#include "aubio/types.h"
#include "Vocoder.h"
#include <iostream>
using namespace std;

class PitchDetector {
    uint_t sampleCounter = 0;
    float currentPitch;
    Vocoder* vocoder;

    uint_t aubioWindow;
    uint_t aubioHopsize;
    uint_t aubioRate;
    fvec_t *aubioInput;
    fvec_t *aubioOutput;
    aubio_pitch_t *aubioPitch;

public:
    PitchDetector(unsigned int sampleRate, unsigned int windowSize);
    void process(float sample);
    float getPitch();
};


#endif //RTPIANO_PITCHDETECTOR_H
