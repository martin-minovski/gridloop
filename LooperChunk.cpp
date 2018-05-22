//
// Created by martin on 4/30/18.
//

#include "LooperChunk.h"

LooperChunk::LooperChunk() {
    for (int i = 0; i < size; i++) {
        samples[i] = 0;
        zonePointers[i] = 0;
        zoneValues[i] = 0;
    }
}
bool LooperChunk::isFull() {
    return writer >= size;
}
void LooperChunk::writeSample(float sample) {
    samples[writer++] = sample;
}
float LooperChunk::getSample(int index) {
    return samples[index];
}
void LooperChunk::setNext(LooperChunk* nextChunk) {
    this->nextChunk = nextChunk;
}
void LooperChunk::setPrev(LooperChunk* prevChunk) {
    this->prevChunk = prevChunk;
}
int LooperChunk::getWriter() {
    return writer;
}
int LooperChunk::getSize() {
    return size;
}
LooperChunk* LooperChunk::getNext() {
    return nextChunk;
}

// Faust widget automation

void LooperChunk::writeZone(long pointer, float value) {
    zonePointers[zoneWriter] = pointer;
    zoneValues[zoneWriter++] = value;
}
void LooperChunk::runWidgetAutomation(int index) {
    if (zonePointers[index] != 0) {
        auto zonePtr = (float*)zonePointers[index];
        *zonePtr = zoneValues[index];
    }
}