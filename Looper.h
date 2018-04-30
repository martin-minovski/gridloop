//
// Created by martin on 4/24/18.
//

#ifndef RTPIANO_LOOPER_H
#define RTPIANO_LOOPER_H

#include <vector>
#include "LooperClip.h"
using namespace std;



class Looper {
    LooperClip* recordingClip = nullptr;
    vector<LooperClip*> clips;
    int activeChannel = 0;
    int timer = 0;
public:
    float process(float sample);
    void startRec();
    void stopRec();
    void setActiveChannel(int channel);
    void schedule(LooperClip* clip);
};


#endif //RTPIANO_LOOPER_H
