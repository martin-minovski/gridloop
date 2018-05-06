//
// Created by martin on 4/28/18.
//

#ifndef RTPIANO_OSC_H
#define RTPIANO_OSC_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include "tinyosc.h"
#include <functional>
#include <iostream>
using namespace std;

class OSC {
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    char buffer[2048]; // declare a 2Kb buffer to read packet data into
    std::function<void(tosc_message*)> oscCallback;

    int fdOut;
    struct addrinfo* addInfoOut = 0;
    char bufferOut[8192];

public:
    OSC(std::function<void(tosc_message*)> oscCallback);
    void oscListen();
    void closeSocket();
    void sendJson(const char* json);
};


#endif //RTPIANO_OSC_H
