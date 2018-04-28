//
// Created by martin on 4/28/18.
//

#include "OSC.h"
using namespace std;


OSC::OSC() {
    fcntl(fd, F_SETFL, O_NONBLOCK); // set the socket to non-blocking
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(4368);
    sin.sin_addr.s_addr = INADDR_ANY;
    bind(fd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in));
    printf("tinyosc is now listening on port 4368.\n");
};

void OSC::setCallback(std::function<void(tosc_message*)> callback) {
    oscCallback = callback;
}

void OSC::oscListen() {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(fd, &readSet);
    struct timeval timeout = {0, 0}; // select times out after 0 seconds (NO TIMEOUT!)
    if (select(fd+1, &readSet, NULL, NULL, &timeout) > 0) {
        struct sockaddr sa; // can be safely cast to sockaddr_in
        socklen_t sa_len = sizeof(struct sockaddr_in);
        int len = 0;
        while ((len = (int) recvfrom(fd, buffer, sizeof(buffer), 0, &sa, &sa_len)) > 0) {
            if (tosc_isBundle(buffer)) {
                tosc_bundle bundle;
                tosc_parseBundle(&bundle, buffer, len);
                const uint64_t timetag = tosc_getTimetag(&bundle);
                tosc_message osc;
                while (tosc_getNextMessage(&bundle, &osc)) {
                    oscCallback(&osc);
                }
            } else {
                tosc_message osc;
                tosc_parseMessage(&osc, buffer, len);
                oscCallback(&osc);
            }
        }
    }
}

void OSC::closeSocket() {
    // close the UDP socket
    close(fd);
}