//
// Created by martin on 4/24/18.
//

#ifndef RTPIANO_LOOPER_H
#define RTPIANO_LOOPER_H

#include <vector>
using namespace std;

class Chunk {
    float* left;
    float* right;
};

class Clip {
    vector<Chunk> chunks;
public:

};

class Looper {
    bool isRecording = false;

public:
    void render(double* inputBuffer, double* outputBuffer, int bufferLength);
    void startRecording();
    void stopRecording();
};


#endif //RTPIANO_LOOPER_H
