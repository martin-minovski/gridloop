//
// Created by martin on 4/24/18.
//

#include <iostream>
#include "Looper.h"

using namespace std;

void Looper::render(double *inputBuffer, double *outputBuffer, int bufferLength) {
//    cout << "Looper rendering" << endl;
}

void Looper::startRecording() {
    if (this->isRecording) return;
    this->isRecording = true;
}

void Looper::stopRecording() {
    if (this->isRecording) return;
    this->isRecording = false;


}