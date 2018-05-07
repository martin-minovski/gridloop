

#include <iostream>

#include "rtaudio/RtAudio.h"
#include "stk/RtMidi.h"

#include "Looper.h"
#include "Vocoder.h"
#include "OSC.h"
#include "AudioEngine.h"
#include "SFSynth.h"
#include "FileManager.h"

//#define MIDI_ENABLED

using namespace std;

Looper* looper;
Vocoder* vocoder;
OSC* osc;
AudioEngine* audioEngine;
SFSynth* sfSynth;
FileManager* fileManager;

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
        inputSample = 0; // disable for now.
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
    else if (address == "piano") {
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
    else if (address == "rec") {
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
    else if (address == "instrument") {
        int bank = tosc_getNextInt32(msg);
        int instrument = tosc_getNextInt32(msg);
        sfSynth->setPreset(bank, instrument);
    }
    else if (address == "looperchannel") {
        int chNum = tosc_getNextInt32(msg);
        looper->setActiveChannel(chNum);
    }
    else if (address == "solochannel") {
        int chNum = tosc_getNextInt32(msg);
        bool solo = tosc_getNextInt32(msg) == 1;
        looper->setChannelSolo(chNum, solo);
    }
    else if (address == "getwidgets") {
        osc->sendJson(looper->getWidgetJSON().c_str());
    }
    else if (address == "updatezone") {
        string zone = tosc_getNextString(msg);
        float value = tosc_getNextFloat(msg);
        float* zonePtr = (float*)stol(zone);
        *zonePtr = value;
    }
    else if (address == "getfaustcode") {
        int channel = tosc_getNextInt32(msg);
        string code = fileManager->getFaustCode(channel);
        osc->sendFaustCode(channel, code.c_str());
    }
    else if (address == "writefaustcode") {
        int channel = tosc_getNextInt32(msg);
        string code = tosc_getNextString(msg);
        fileManager->writeFaustCode(channel, code);
        if (looper->reloadChannelDSP(channel)) osc->sendFaustAck();
    }
}

int main() {

    // Audio parameters
    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 128;


    sfSynth = new SFSynth(sampleRate * 2, bufferFrames);
    osc = new OSC(oscCallback);
    looper = new Looper(osc);
    vocoder = new Vocoder();
    fileManager = new FileManager();
    audioEngine = new AudioEngine(sampleRate, bufferFrames, &render, RtAudio::UNIX_JACK);


    // Initialize MIDI listener
#ifdef MIDI_ENABLED
    RtMidiIn *midiIn = new RtMidiIn();
    // Check available ports.
    unsigned int nPorts = midiIn->getPortCount();
    if ( nPorts == 0 ) {
        std::cout << "No ports available!\n";
    }
    else {

        std::cout << nPorts << " ports available!\n";

        midiIn->openPort( 1 );
        // Set our callback function.  This should be done immediately after
        // opening the port to avoid having incoming messages written to the
        // queue.
        midiIn->setCallback( &midiCallback );
        // Ignore sysex, timing, or active sensing messages:
        midiIn->ignoreTypes( true, true, true );
    }
#endif


    // Quit scenario

    char input;
    std::cout << "\nRunning ... press <enter> to quit.\n";
    std::cin.get( input );

    audioEngine->shutDown();
    osc->closeSocket();

    return 0;
}