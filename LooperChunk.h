//
// Created by martin on 4/30/18.
//

#ifndef RTPIANO_LOOPERCHUNK_H
#define RTPIANO_LOOPERCHUNK_H

#define CHUNK_SIZE 1024

class LooperChunk {
    int size = CHUNK_SIZE;
    int writer = 0;
    float samples[CHUNK_SIZE];
    LooperChunk* nextChunk = nullptr;
    LooperChunk* prevChunk = nullptr;

    // Faust widget automation
    int zoneWriter = 0;
    long zonePointers[CHUNK_SIZE];
    float zoneValues[CHUNK_SIZE];
public:
    LooperChunk();
    bool isFull();
    void writeSample(float sample);
    float getSample(int index);
    void setNext(LooperChunk* nextChunk);
    void setPrev(LooperChunk* prevChunk);
    int getWriter();
    int getSize();
    LooperChunk* getNext();
    void roundOut();
    void roundIn();

    void writeZone(long pointer, float value);
    void runWidgetAutomation(int index);
};


#endif //RTPIANO_LOOPERCHUNK_H
