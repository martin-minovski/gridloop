//
// Created by martin on 4/28/18.
//

#include "Vocoder.h"

Vocoder::Vocoder() {
    // Init shiftedBy
    for (int n = 0; n < fftSize; n++) {
        prevFrame[n].shiftedBy = 0;
    }

    // Calculate a Hann window
    for (int n = 0; n < fftSize; n++) {
//        hannWindow[n] = 0.5 * (1 - cos(2 * M_PI * j / (fftSize - 1)));
        hannWindow[n] = 0.5f * (1.0f - cosf(2.0 * M_PI * n / (float) (fftSize - 1)));
    }

    // Init KissFFT
    inFFT = kiss_fft_alloc(fftSize, 0, NULL, NULL);
    outFFT = kiss_fft_alloc(fftSize, 1, NULL, NULL);
    for (int i = 0; i < fftSize; i++) {
        cpxBufferOut[i].r = 0.0f;
        cpxBufferOut[i].i = 0.0f;
        cpxBufferIn[i].r = 0.0f;
        cpxBufferIn[i].i = 0.0f;
        cpxBuffer[i].r = 0.0f;
        cpxBuffer[i].i = 0.0f;
        assemblyBuffer[i] = 0.0f;
    }
    for (int i = 0; i < olaSize; i++) {
        olaBuffer[i] = 0.0f;
    }
}


float Vocoder::processSample(float &sample) {
    if (hopCounter >= hopSize) {
        hopCounter = 0;

        // Shift assembly
        for (int j = hopSize; j < fftSize; j++) {
            assemblyBuffer[j - hopSize] = assemblyBuffer[j];
        }
        // Fill assembly
        for (int j = 0; j < hopSize; j++) {
            assemblyBuffer[fftSize - hopSize + j] = hopBuffer[j];
        }

        // Copy to cpx input buffer
        for (int j = 0; j < fftSize; j++) {
            cpxBufferIn[j].r = assemblyBuffer[j];
        }

        // Hann window
        for (int j = 0; j < fftSize; j++) {
            cpxBufferIn[j].r = hannWindow[j] * cpxBufferIn[j].r;
        }

        kiss_fft(inFFT, cpxBufferIn, cpxBuffer);

        processFrequencyDomain(cpxBuffer);

        kiss_fft(outFFT, cpxBuffer, cpxBufferOut);

        // Scale and overlap-add
        for (int j = 0; j < fftSize; j++) {
            cpxBufferOut[j].r *= 0.0005f;
            olaBuffer[(olaWrite + j) % olaSize] += cpxBufferOut[j].r;
        }
        olaWrite = (olaWrite + hopSize) % olaSize;
    }

    float fftResultSample = olaBuffer[olaRead];
    olaBuffer[olaRead] = 0;
    if (++olaRead >= olaSize) olaRead = 0;

    hopBuffer[hopCounter] = (float)sample;
    hopCounter++;

    return fftResultSample;
}

void Vocoder::processFrequencyDomain(kiss_fft_cpx *cpx) {

    float localBetaFactor = gBetaFactor;

    // Initialize
    for (int n = 0; n < fftSize; n++) {
        mag[n] = sqrtf(cpx[n].r * cpx[n].r + cpx[n].i * cpx[n].i);

        nextFrame[n].complex.r = cpx[n].r;
        nextFrame[n].complex.i = cpx[n].i;
        nextFrame[n].peak = 0;
    }

    // Get magnitudes and pinpoint peaks
    for (int n = 2; n < fftSize - 2; n++) {
        bool isPeak = (mag[n] > mag[n-1]) && (mag[n] > mag[n-2]) && (mag[n] > mag[n+1]) && (mag[n] > mag[n+2]);
        if (isPeak) {
            nextFrame[n].peak = 1;
        }
    }

    // Identify regions
    float lowestPoint = std::numeric_limits<float>::max();
    int lastSaddle = 0;
    int lastPeak = 0;
    for (int n = 0; n < fftSize; n++) {
        if (nextFrame[n].peak && n < fftSize) {
            for (int m = lastPeak; m < lastSaddle; m++) {
                nextFrame[m].peak = lastPeak;
            }
            for (int m = lastSaddle; m < n; m++) {
                nextFrame[m].peak = n;
            }
            lastPeak = n;
            lowestPoint = mag[n];
        }
        else {
            if (mag[n] < lowestPoint) {
                lowestPoint = mag[n];
                lastSaddle = n;
            }
            // End scenario
            if (n == fftSize-1) {
                for (int m = lastPeak; m < fftSize; m++) {
                    nextFrame[m].peak = lastPeak;
                }
            }
        }
    }

    // Prepare nextShifted
    for (int n = 0; n < fftSize; n++) {
        nextShifted[n].complex.i = 0.0f;
        nextShifted[n].complex.r = 0.0f;
        nextShifted[n].shiftedBy = 0;
        nextShifted[n].peak = 0;
    }

    // Shift peaks
    for (int n = 0; n < fftSize; n++) {
        float precisePeakLocation = localBetaFactor * nextFrame[n].peak;
        int newPeakLocation = (int)roundf(precisePeakLocation);
        int shiftValue = newPeakLocation - nextFrame[n].peak;

        int shiftIndex = n + shiftValue;

        if (shiftIndex >= 0 && shiftIndex < fftSize) {
            nextShifted[shiftIndex].complex.r += nextFrame[n].complex.r;
            nextShifted[shiftIndex].complex.i += nextFrame[n].complex.i;
            nextShifted[shiftIndex].shiftedBy = shiftValue;
            nextShifted[shiftIndex].peak = newPeakLocation;
        }
    }

    // Adjust phases
    for (int n = 0; n < fftSize; n++) {
        if (nextShifted[n].peak && prevFrame[nextShifted[n].peak].shiftedBy % 2 != 0) {
            if (phaseLock) {
                nextShifted[n].complex.i = -nextShifted[n].complex.i;
                nextShifted[n].complex.r = -nextShifted[n].complex.r;
            }
        }
        prevFrame[n].shiftedBy += nextShifted[n].shiftedBy;
        prevFrame[n].shiftedBy %= 2;
    }

    // Submit
    for (int n = 0; n < fftSize; n++) {
        cpx[n].i = nextShifted[n].complex.i;
        cpx[n].r = nextShifted[n].complex.r;
    }

}