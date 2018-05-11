//
// Created by martin on 4/28/18.
//

#include "Vocoder.h"

Vocoder::Vocoder() {
    // Init phase differences
    for (int n = 0; n < fftSize; n++) {
        phaseDifference[n] = 0;
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

        // Frame-synchronous time-domain amplitude modulation for the peak-shift linear interpolation
        for (int n = 0; n < fftSize; n++) {
            cpxBufferOut[n].r = cpxBufferOut[n].r * sinf((float)M_PI * ((float)n / (float)fftSize));
        }

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

float getMagnitude(kiss_fft_cpx& complex) {
    return sqrtf(complex.i * complex.i + complex.r * complex.r);
}

bool isEmpty(kiss_fft_cpx& complex) {
    return complex.i == 0.0f && complex.r == 0.0f;
}

void Vocoder::processFrequencyDomain(kiss_fft_cpx *cpx) {

    float localBetaFactor = gBetaFactor;

    // Initialize
    for (int n = 0; n < fftSize; n++) {

        nextFrame[n].complex.r = cpx[n].r;
        nextFrame[n].complex.i = cpx[n].i;
        nextFrame[n].peak = 0;

        std::complex<float> ccpx = {cpx[n].r, cpx[n].i};
        nextFrame[n].abs = abs(ccpx);
        nextFrame[n].arg = arg(ccpx);
        mag[n] = abs(ccpx);
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

        nextShifted[n].abs = 0;
        nextShifted[n].arg = 0;
    }

    // Shift peaks
    for (int n = 0; n < fftSize; n++) {
        if (nextFrame[n].peak > 3 * fftSize / 4) continue; // Fix high pitch noise

        float precisePeakLocation = localBetaFactor * nextFrame[n].peak;
        int newPeakLocation = (int)round(precisePeakLocation);
        int shiftValue = newPeakLocation - nextFrame[n].peak;

        int shiftIndex = n + shiftValue;


        float x = newPeakLocation - precisePeakLocation;

        if (shiftIndex >= 0 && shiftIndex < fftSize) {
            if (x >= 0 && n > 0) {
                nextShifted[shiftIndex].abs = nextFrame[n-1].abs * x + nextFrame[n].abs * (1-x);
            }
            if (x < 0 && n < fftSize-1) {
                nextShifted[shiftIndex].abs = nextFrame[n+1].abs * (-x) + nextFrame[n].abs * (1-(-x));
            }
            nextShifted[shiftIndex].complex.i = nextFrame[n].arg;
            nextShifted[shiftIndex].shiftedBy = precisePeakLocation - nextFrame[n].peak;
//            nextShifted[shiftIndex].abs = nextFrame[n].abs;
//            nextShifted[shiftIndex].arg = nextFrame[n].arg;
//            nextShifted[shiftIndex].shiftedBy = shiftValue;
//            nextShifted[shiftIndex].peak = newPeakLocation;
        }
    }

    float tempDiff[fftSize];

    // Adjust phases
    for (int n = 0; n < fftSize; n++) {
        if (stateSwitch) {
            float magnitude = nextShifted[n].abs;
            float diff = phaseDifference[nextShifted[n].peak] + ((2 * (float)M_PI * nextShifted[n].shiftedBy) / fftSize) * hopSize;
            std::complex<float> ccpx = polar(magnitude, nextShifted[n].arg + diff);
            nextShifted[n].complex.r = real(ccpx);
            nextShifted[n].complex.i = imag(ccpx);

            tempDiff[n] = diff;
        }
    }
    for (int n = 0; n < fftSize; n++) {
        phaseDifference[n] = tempDiff[n];
    }

    // Submit
    for (int n = 0; n < fftSize; n++) {
        cpx[n].i = nextShifted[n].complex.i;
        cpx[n].r = nextShifted[n].complex.r;
    }
}

void Vocoder::setBetaFactor(float newBetaFactor) {
    gBetaFactor = newBetaFactor;
    cout<<"Beta: "<<newBetaFactor<<endl;
}

void Vocoder::switchState(bool state) {
    stateSwitch = state;
}