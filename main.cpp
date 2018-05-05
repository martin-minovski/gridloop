

#include <iostream>

#include "rtaudio/RtAudio.h"
#include "stk/RtMidi.h"

#include "Looper.h"
#include "Vocoder.h"
#include "OSC.h"
#include "AudioEngine.h"
#include "SFSynth.h"

using namespace std;

Looper* looper;
Vocoder* vocoder;
OSC* osc;
AudioEngine* audioEngine;
SFSynth* sfSynth;
bool looperListening = false;

void midiCallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{

    unsigned int nBytes = message->size();
    for ( unsigned int i=0; i<nBytes; i++ )
        std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
    if ( nBytes > 0 )
        std::cout << "stamp = " << deltatime << std::endl;


    if (message->at(0) == 144) {
        int pitch = message->at(1);
        if (message->at(2) == 0) sfSynth->noteOff(pitch);
        else sfSynth->noteOn(pitch, message->at(2));
    }

    if (message->at(0) == 176) {
        if (message->at(2) > 0) sfSynth->sustainOn();
        else sfSynth->sustainOff();
    }
}


int render(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void *userData) {

    double *inBuffer = (double *) inputBuffer;
    double *outBuffer = (double *) outputBuffer;

    if (status) {
        std::cout << "Stream underflow detected!" << std::endl;
    }

    for (int i = 0; i < nBufferFrames * 2; i++) {
        float tsfSample = sfSynth->getNextSample();
        float inputSample = (float) inBuffer[(i / 2) * 2];
        float looperSample = looper->process(tsfSample + inputSample);
        outBuffer[i] = tsfSample + looperSample + inputSample;

//        float fftSample = 0;
//        if (i % 2 == 0) fftSample = vocoder->processSample(tsfSample);
//        else fftSample = tsfSample;
//        outBuffer[i] = fftSample;
    }

    osc->oscListen();

    return 0;
}

void oscCallback(tosc_message* msg) {

//    tosc_printMessage(msg);

    string address = tosc_getAddress(msg);

    if (address == "slider") {
        string type = tosc_getNextString(msg);
        float value = tosc_getNextFloat(msg);
        int meta = tosc_getNextInt32(msg);
        if (type == "vocoder") {
            vocoder->setBetaFactor(value);
        }
        if (type == "sfgain") {
            sfSynth->setGain(value);
        }
        if (type == "channelvolume") {
            looper->setChannelVolume(meta, value);
        }
    }
    if (address == "piano") {
        int pitch = tosc_getNextInt32(msg);
        int state = tosc_getNextInt32(msg);
        int octaveShift = tosc_getNextInt32(msg);
        if (state == 1) {
            sfSynth->noteOn(pitch + octaveShift * 12, 100);
            if (looperListening) {
                looper->startRec();
                looperListening = false;
            }
        }
        else {
            sfSynth->noteOff(pitch + octaveShift * 12);
        }
    }
    if (address == "rec") {
        int state = tosc_getNextInt32(msg);
        int directRec = tosc_getNextInt32(msg);
        if (state == 1) {
            if (directRec == 1) {
                looperListening = false;
                looper->startRec();
            }
            else looperListening = true;
        }
        else {
            looperListening = false;
            looper->stopRec();
        }
    }
    if (address == "instrument") {
        int bank = tosc_getNextInt32(msg);
        int instrument = tosc_getNextInt32(msg);
        sfSynth->setPreset(bank, instrument);
    }
    if (address == "looperchannel") {
        int chNum = tosc_getNextInt32(msg);
        looper->setActiveChannel(chNum);
    }
    if (address == "solochannel") {
        int chNum = tosc_getNextInt32(msg);
        bool solo = tosc_getNextInt32(msg) == 1;
        looper->setChannelSolo(chNum, solo);
    }
}

int main() {

    // Audio parameters
    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 128;


    sfSynth = new SFSynth(sampleRate * 2, bufferFrames);
    looper = new Looper();
    vocoder = new Vocoder();
    osc = new OSC(oscCallback);
    audioEngine = new AudioEngine(sampleRate, bufferFrames, &render, RtAudio::UNIX_JACK);


    // Initialize MIDI listener


//    RtMidiIn *midiIn = new RtMidiIn();
//    // Check available ports.
//    unsigned int nPorts = midiIn->getPortCount();
//    if ( nPorts == 0 ) {
//        std::cout << "No ports available!\n";
//    }
//    else {
//
//        std::cout << nPorts << " ports available!\n";
//
//        midiIn->openPort( 1 );
//        // Set our callback function.  This should be done immediately after
//        // opening the port to avoid having incoming messages written to the
//        // queue.
//        midiIn->setCallback( &midiCallback );
//        // Ignore sysex, timing, or active sensing messages:
//        midiIn->ignoreTypes( true, true, true );
//    }

    // Quit scenario

    char input;
    std::cout << "\nRunning ... press <enter> to quit.\n";
    std::cin.get( input );

    audioEngine->shutDown();
    osc->closeSocket();

    return 0;
}