//
// LoopGrid by Martin Minovski, 2018
//

#include "OSC.h"
using namespace std;

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/uio.h>


OSC::OSC(std::function<void(tosc_message*)> callback) {

    //Set up inbound socket
    fcntl(fd, F_SETFL, O_NONBLOCK); // set the socket to non-blocking
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(4368);
    sin.sin_addr.s_addr = INADDR_ANY;
    bind(fd, (struct sockaddr *) &sin, sizeof(struct sockaddr_in));
    printf("Listening for OSC on port 4368.\n");
    oscCallback = callback;

    // Set up outbound socket
    const char* hostnameOut = "127.0.0.1";
    const char* portnameOut = "10295";
    struct addrinfo hints;
    memset(&hints,0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_ADDRCONFIG;
    int err = getaddrinfo(hostnameOut, portnameOut, &hints, &addInfoOut);
    if (err != 0) {
        printf("Failed to resolve remote socket address. (err:%d)\n", err);
        return;
    }
    // Open socket
    fdOut = socket(addInfoOut->ai_family, addInfoOut->ai_socktype, addInfoOut->ai_protocol);
    if (fdOut == -1) {
        printf("Error: %s\n", strerror(errno));
        return;
    }

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
    // close the UDP sockets
    close(fd);
    close(fdOut);
}
void OSC::sendJson(const char* json) {
    unsigned int len = tosc_writeMessage(
            bufferOut, sizeof(bufferOut),
            "json_update",
            "s",
            json);
    sendto(fd, bufferOut, len, 0, addInfoOut->ai_addr, addInfoOut->ai_addrlen);
}
void OSC::sendFaustError(const char *errorMsg) {
    unsigned int len = tosc_writeMessage(
            bufferOut, sizeof(bufferOut),
            "faust_error",
            "s",
            errorMsg);
    sendto(fd, bufferOut, len, 0, addInfoOut->ai_addr, addInfoOut->ai_addrlen);
}
void OSC::sendFaustCode(int channel, const char *code) {
    unsigned int len = tosc_writeMessage(
            bufferOut, sizeof(bufferOut),
            "faust_code",
            "is",
            channel, code);
    sendto(fd, bufferOut, len, 0, addInfoOut->ai_addr, addInfoOut->ai_addrlen);
}
void OSC::sendFaustAck() {
    unsigned int len = tosc_writeMessage(
            bufferOut, sizeof(bufferOut),
            "faust_ack",
            "");
    sendto(fd, bufferOut, len, 0, addInfoOut->ai_addr, addInfoOut->ai_addrlen);
}
void OSC::sendInstruments(string instruments) {
    unsigned int len = tosc_writeMessage(
            bufferOut, sizeof(bufferOut),
            "json_instruments",
            "s",
            instruments.c_str());
    sendto(fd, bufferOut, len, 0, addInfoOut->ai_addr, addInfoOut->ai_addrlen);
}
void OSC::sendClipSummary(string json) {
    unsigned int len = tosc_writeMessage(
            bufferOut, sizeof(bufferOut),
            "json_clipsummary",
            "s",
            json.c_str());
    sendto(fd, bufferOut, len, 0, addInfoOut->ai_addr, addInfoOut->ai_addrlen);
}
void OSC::sendChannelSummary(string json) {
    unsigned int len = tosc_writeMessage(
            bufferOut, sizeof(bufferOut),
            "json_channelsummary",
            "s",
            json.c_str());
    sendto(fd, bufferOut, len, 0, addInfoOut->ai_addr, addInfoOut->ai_addrlen);
}
void OSC::sendActive(int channel, int variation) {
    unsigned int len = tosc_writeMessage(
            bufferOut, sizeof(bufferOut),
            "active",
            "ii",
            channel, variation);
    sendto(fd, bufferOut, len, 0, addInfoOut->ai_addr, addInfoOut->ai_addrlen);
}
void OSC::sendRecUpdate(bool state) {
    unsigned int len = tosc_writeMessage(
            bufferOut, sizeof(bufferOut),
            "recstate_update",
            "i",
            (int)state);
    sendto(fd, bufferOut, len, 0, addInfoOut->ai_addr, addInfoOut->ai_addrlen);
}