//
// Created by martin on 5/2/18.
//

#ifndef RTPIANO_LOOPERCHANNEL_H
#define RTPIANO_LOOPERCHANNEL_H


class LooperChannel {
    int id;
    float volume = 1.0f;
public:
    bool solo = false;
    bool soloMute = false;
    LooperChannel(int id);
    float process(float sample);
    void setVolume(float volume);
};


#endif //RTPIANO_LOOPERCHANNEL_H
