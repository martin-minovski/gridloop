//
// LoopGrid by Martin Minovski, 2018
// NOTE: This is part of a SMSA miniproject
//

#include "Vocoder.h"

Vocoder::Vocoder() {
    // Init previous frame
    for (int n = 0; n < fftSize; n++) {
        prevFrame[n].shiftedBy = 0;
        prevFrame[n].phase = 0;
        prevFrame[n].peak = 0;
    }

    // Calculate a Hann window
    for (int n = 0; n < fftSize; n++) {
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
        mag[n] = sqrtf(cpx[n].r * cpx[n].r + cpx[n].i * cpx[n].i);

        nextFrame[n].complex.r = cpx[n].r;
        nextFrame[n].complex.i = cpx[n].i;
        nextFrame[n].peak = 0;
        nextFrame[n].precisePeak = 0;
        nextFrame[n].phase = 0;
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

    // Estimate precise peak location
    for (int n = 0; n < fftSize; n++) {
        if (parabolaFit) {
            if (nextFrame[n].peak == n) {
                // Quadratic parabola
                float d = (mag[n+1] - mag[n-1]) / (2 * (2 * mag[n] - mag[n-1] - mag[n+1]));
                nextFrame[n].precisePeak = n + d;
            }
        }
        else {
            nextFrame[n].precisePeak = n;
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
        if (nextFrame[n].peak > 3 * fftSize / 4) continue; // Fix high pitch noise

        float precisePeakLocation = localBetaFactor * nextFrame[nextFrame[n].peak].precisePeak;
        int newPeakLocation = (int)round(precisePeakLocation);
        int shiftValue = newPeakLocation - nextFrame[n].peak;
        int shiftIndex = n + shiftValue;

        // Apply linear interpolation
        float x = newPeakLocation - precisePeakLocation;
        if (shiftIndex >= 0 && shiftIndex < fftSize) {
            if (linearInterpolation) {
                if (x >= 0 && n > 0) {
                    float additionR = 0;
                    float additionI = 0;
                    if (nextFrame[n].peak == nextFrame[n - 1].peak) {
                        additionR = (nextFrame[n - 1].complex.r * x);
                        additionI = (nextFrame[n - 1].complex.i * x);
                    }
                    nextShifted[shiftIndex].complex.r += additionR + (nextFrame[n].complex.r * (1.0f - x));
                    nextShifted[shiftIndex].complex.i += additionI + (nextFrame[n].complex.i * (1.0f - x));
                }
                if (x < 0 && n < fftSize - 1) {
                    float additionR = 0;
                    float additionI = 0;
                    if (nextFrame[n].peak == nextFrame[n + 1].peak) {
                        additionR = (nextFrame[n + 1].complex.r * -x);
                        additionI = (nextFrame[n + 1].complex.i * -x);
                    }
                    nextShifted[shiftIndex].complex.r += additionR + (nextFrame[n].complex.r * (1.0f + x));
                    nextShifted[shiftIndex].complex.i += additionI + (nextFrame[n].complex.i * (1.0f + x));
                }
                nextShifted[shiftIndex].shiftedBy = precisePeakLocation - nextFrame[nextFrame[n].peak].precisePeak;
                nextShifted[shiftIndex].peak = newPeakLocation;
            }
            else {
                nextShifted[shiftIndex].complex.r += nextFrame[n].complex.r;
                nextShifted[shiftIndex].complex.i += nextFrame[n].complex.i;
                nextShifted[shiftIndex].shiftedBy = shiftValue;
                nextShifted[shiftIndex].peak = newPeakLocation;
            }
        }
    }

    // Adjust phases
    for (int n = 0; n < fftSize; n++) {
        std::complex<float> ccpx(nextShifted[n].complex.r, nextShifted[n].complex.i);
        float magnitude = abs(ccpx);

            // Temporary crash workaround
            int nextPeak = nextShifted[n].peak;
            if (nextPeak < 0 || nextPeak >= fftSize) continue;

        float phase =
                prevFrame[nextPeak].phase
                + ((2 * (float)M_PI * nextShifted[n].shiftedBy) / fftSize) * hopSize;
        ccpx = polar(magnitude, phase + arg(ccpx));
        nextShifted[n].complex.r = real(ccpx);
        nextShifted[n].complex.i = imag(ccpx);
        nextShifted[n].phase = phase;
    }
    for (int n = 0; n < fftSize; n++) {
        prevFrame[n].phase = nextShifted[n].phase;
        prevFrame[n].peak = nextShifted[n].peak;
    }

    // Submit
    for (int n = 0; n < fftSize; n++) {
        cpx[n].i = nextShifted[n].complex.i;
        cpx[n].r = nextShifted[n].complex.r;
    }
}

void Vocoder::setBetaFactor(float newBetaFactor) {
    gBetaFactor = newBetaFactor;
}

void Vocoder::setLinearInterpolation(bool value) {
    linearInterpolation = value;
}

void Vocoder::setParabolaFit(bool value) {
    parabolaFit = value;
}