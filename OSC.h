//
// Created by martin on 4/28/18.
//

#ifndef RTPIANO_OSC_H
#define RTPIANO_OSC_H
#define IN_BUFFER_SIZE 8192
#define OUT_BUFFER_SIZE 32768
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include "tinyosc.h"
#include <functional>
#include <iostream>
using namespace std;

class OSC {
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    char buffer[IN_BUFFER_SIZE];
    std::function<void(tosc_message*)> oscCallback;

    int fdOut;
    struct addrinfo* addInfoOut = 0;
    char bufferOut[OUT_BUFFER_SIZE];

public:
    OSC(std::function<void(tosc_message*)> oscCallback);
    void oscListen();
    void closeSocket();

    // Outbound
    void sendJson(const char* json);
    void sendFaustError(const char* errorMsg);
    void sendFaustCode(int channel, const char *code);
    void sendFaustAck();
    void sendInstruments(string instruments);
};


#endif //RTPIANO_OSC_H
