//
// Created by martin on 4/28/18.
//

#ifndef RTPIANO_SFSYNTH_H

#define RTPIANO_SFSYNTH_H


class SFSynth {
    long long tsfPtr;
    float buffer[4096];
    int bufferSize;
    int bufferCounter = 0;
    int sustained[127];
    bool isSustaining = false;
    int tsfPreset = 0;
    float gain = 0.5;
public:
    SFSynth(unsigned int sampleRate, unsigned int bufferSize);
    void setPreset(int preset);
    void noteOn(int pitch, int velocity);
    void noteOff(int pitch);
    void sustainOn();
    void sustainOff();
    void setGain(float gain);
    float getNextSample();
};

#endif //RTPIANO_SFSYNTH_H
