//
// Created by martin on 4/28/18.
//

#ifndef RTPIANO_VOCODER_H
#define RTPIANO_VOCODER_H

#include "kiss_fft.h"
#include <limits>

struct Bin
{
    kiss_fft_cpx complex;
    int peak;
    int shiftedBy;
};

const int fftSize = 4096;
const int hopSize = 2048;
const int olaSize = fftSize + hopSize;

class Vocoder {

    Bin prevFrame[fftSize];
    Bin nextFrame[fftSize];
    Bin nextShifted[fftSize];
    float mag[fftSize];

    // TODO: NexusUI
    bool phaseLock = true;
    float gBetaFactor = 2.0f;

    kiss_fft_cfg inFFT;
    kiss_fft_cfg outFFT;
    float assemblyBuffer[fftSize];
    kiss_fft_cpx cpxBufferIn[fftSize];
    kiss_fft_cpx cpxBuffer[fftSize];
    kiss_fft_cpx cpxBufferOut[fftSize];

    int hopCounter = 0;
    float hopBuffer[hopSize];

    float olaBuffer[olaSize];
    int olaRead = 0;
    int olaWrite = hopSize;
    float hannWindow[fftSize];

public:
    Vocoder();
    float processSample(float &sample);
    void processFrequencyDomain(kiss_fft_cpx* cpx);
    void setBetaFactor(float newBetaFactor);

};



#endif //RTPIANO_VOCODER_H
