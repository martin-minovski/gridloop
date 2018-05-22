

#include <iostream>

#include "rtaudio/RtAudio.h"
#include "stk/RtMidi.h"

#include "Looper.h"
#include "Vocoder.h"
#include "OSC.h"
#include "AudioEngine.h"
#include "SFSynth.h"
#include "FileManager.h"
#include "PitchDetector.h"
#include <cmath>

#define MIDI_ENABLED

using namespace std;

Looper* looper;
Vocoder* vocoder;
Vocoder* vocoder2;
Vocoder* vocoder3;
OSC* osc;
AudioEngine* audioEngine;
FileManager* fileManager;
PitchDetector* pitchDetector;

bool looperListening = false;
bool vocoderEnabled = false;

void midiCallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
//    unsigned int nBytes = message->size();
//    for ( unsigned int i=0; i<nBytes; i++ )
//        std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
//    if ( nBytes > 0 )
//        std::cout << "stamp = " << deltatime << std::endl;
    int pitch = message->at(1);
    if (pitch == 1) {
        vocoder->setBetaFactor(0.95f + ((float)message->at(2) / 1280.0f));
    }
//    else if (pitch == 21) {
//
//    }
//    else if (pitch == 22) {
//
//    }
    else {
        if (message->at(2) == 0) SFSynth::noteOff(pitch);
        else  {
            SFSynth::noteOn(pitch, message->at(2));
            if (looperListening) {
                looper->startRec();
                looperListening = false;
            }
        }
    }
//    if (message->at(0) == 144 || message->at(0) == 158) {
//    }
//    if (message->at(0) == 176) {
//        if (message->at(2) > 0) SFSynth::sustainOn();
//        else SFSynth::sustainOff();
//    }
}

float targetFrequency = 500;

int render(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void *userData) {

    double *inBuffer = (double *) inputBuffer;
    double *outBuffer = (double *) outputBuffer;

    if (status) {
        std::cout << "Stream underflow detected!" << std::endl;
    }

    float fftSample = 0;
    for (int i = 0; i < nBufferFrames * 2; i++) {
        float tsfSample = SFSynth::getNextSample();
        float inputSample = (float) inBuffer[(i / 2) * 2];
        inputSample += tsfSample;

        float looperSample = looper->process(tsfSample + inputSample);
        outBuffer[i] = looperSample;

        if (vocoderEnabled) {
            if (i % 2 == 0) {
                fftSample = vocoder->processSample(looperSample);
//              pitchDetector->process(inputSample + fftSample);
            }
            outBuffer[i] += fftSample;
        }
    }

//    targetFrequency = 196.00;
//    float targetFrequency2 = 293.66;
//    float targetFrequency3 = 493.88;
//    float currentPitch = pitchDetector->getPitch();
//
//    float ratio = targetFrequency / currentPitch;
//    float ratio2 = targetFrequency2 / currentPitch;
//    float ratio3 = targetFrequency3 / currentPitch;
//
//    if (currentPitch > 5 && currentPitch < 2000) {
//        vocoder->setBetaFactor(ratio);
//        vocoder2->setBetaFactor(ratio2);
//        vocoder3->setBetaFactor(ratio3);
//    }

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
            SFSynth::setGain(value);
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
            SFSynth::noteOn(pitch + octaveShift * 12, 100);
            if (looperListening) {
                looper->startRec();
                looperListening = false;
            }
            targetFrequency = (float)(pow(2, ((float)(pitch + octaveShift * 12)-69)/12) * 440);
        }
        else {
            SFSynth::noteOff(pitch + octaveShift * 12);
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

            // Send clip update to frontend
            osc->sendClipSummary(looper->getClipSummary().dump());
        }
    }
    else if (address == "instrument") {
        int bank = tosc_getNextInt32(msg);
        int instrument = tosc_getNextInt32(msg);
        SFSynth::setPreset(bank, instrument);
    }
    else if (address == "looperchannel") {
        int chNum = tosc_getNextInt32(msg);
        int varNum = tosc_getNextInt32(msg);
        looper->setActiveChannel(chNum);
        looper->setActiveVariation(varNum);
        osc->sendActive(looper->getActiveChannel(), looper->getActiveVariation());
    }
    else if (address == "clearchannel") {
        int chNum = tosc_getNextInt32(msg);
        int varNum = tosc_getNextInt32(msg);
        looper->clearChannel(chNum, varNum);
        osc->sendClipSummary(looper->getClipSummary().dump());
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
    else if (address == "vocoderswitch") {
        bool state = tosc_getNextInt32(msg) == 1;
        vocoder->switchState(state);
        vocoderEnabled = state;
    }
    else if (address == "getinstruments") {
        osc->sendInstruments(SFSynth::getInstruments().dump());
    }
    else if (address == "getclipsummary") {
        osc->sendClipSummary(looper->getClipSummary().dump());
    }
    else if (address == "getchannelsummary") {
        osc->sendChannelSummary(looper->getChannelSummary().dump());
    }
    else if (address == "getactive") {
        osc->sendActive(looper->getActiveChannel(), looper->getActiveVariation());
    }
}

int main() {
    // Audio parameters
    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 512;

    SFSynth::init(sampleRate * 2, 256);
    osc = new OSC(oscCallback);
    looper = new Looper(osc);
    vocoder = new Vocoder();
//    vocoder2 = new Vocoder();
//    vocoder3 = new Vocoder();
    pitchDetector = new PitchDetector(sampleRate, 2048);
    fileManager = new FileManager();
    audioEngine = new AudioEngine(sampleRate, bufferFrames, &render, RtAudio::UNIX_JACK);

#ifdef MIDI_ENABLED
    // Initialize MIDI listener
    RtMidiIn *midiIn = new RtMidiIn();
    // Check available ports.
    unsigned int nPorts = midiIn->getPortCount();
    if ( nPorts == 0 ) {
        std::cout << "No ports available!\n";
    }
    else {
        std::cout << nPorts << " MIDI port(s) available\n";
        midiIn->openPort(nPorts - 1);
        midiIn->setCallback(&midiCallback);

        // Ignore sysex, timing, or active sensing messages:
        midiIn->ignoreTypes(true, true, true);
    }
#endif

    // Update front-end on boot
    osc->sendJson(looper->getWidgetJSON().c_str());
    osc->sendInstruments(SFSynth::getInstruments().dump());
    osc->sendClipSummary(looper->getClipSummary().dump());
    osc->sendChannelSummary(looper->getChannelSummary().dump());
    osc->sendActive(looper->getActiveChannel(), looper->getActiveVariation());

    // Quit scenario
    char input;
    std::cout << "\nRunning ... press <enter> to quit.\n";
    std::cin.get( input );
    audioEngine->shutDown();
    osc->closeSocket();
    return 0;
}