//
// LoopGrid by Martin Minovski, 2018
//

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
#include "Theremin.h"
#include <cmath>
#include "AudioFile.h"

#define MIDI_ENABLED
//#define MIDI_DEBUG

using namespace std;

Looper* looper;
Vocoder* vocoder;
OSC* osc;
AudioEngine* audioEngine;
FileManager* fileManager;
PitchDetector* pitchDetector;

bool looperListening = false;
bool vocoderEnabled = false;
bool autotuneEnabled = true;
float masterVolume = 1.0f;

void midiCallback(double deltatime, vector<unsigned char> *message, void *userData)
{
#ifdef MIDI_DEBUG
    unsigned long nBytes = message->size();
    for (unsigned long i = 0; i < nBytes; i++) {
        cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
    }
    if (nBytes > 0) {
        cout << "stamp = " << deltatime << endl;
    }
#endif
    int pitch = message->at(1);
    if (pitch == 1) {
        vocoder->setBetaFactor(0.95f + ((float)message->at(2) / 1280.0f));
    }
    else if (message->at(0) == 176) {
        if (message->at(2) > 0) SFSynth::sustainOn();
        else SFSynth::sustainOff();
    }
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
}

float targetFrequency = 440;

AudioFile<float> audioFile;
int wavSample = 0;

int render(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void *userData) {

    double *inBuffer = (double *) inputBuffer;
    double *outBuffer = (double *) outputBuffer;

    if (status) {
        cout << "Stream underflow detected!" << endl;
    }

    // Extend WAV buffer
    audioFile.setNumSamplesPerChannel(wavSample + nBufferFrames);

    float fftSample = 0;
    for (int i = 0; i < nBufferFrames * 2; i++) {
        int channel = i % 2;
        int sample = i / 2;

        float tsfSample = SFSynth::getNextSample();

        // Get left input channel only
        float inputSample = (float) inBuffer[(sample) * 2];

        // Mix
        inputSample += tsfSample;

        if (vocoderEnabled) {
            // Apply only on left channel
            if (channel == 0) {
                fftSample = vocoder->processSample(inputSample);
                pitchDetector->process(inputSample);
            }
            inputSample += fftSample;
        }

        float looperSample = looper->process(inputSample);
        outBuffer[i] = looperSample * masterVolume;

        // Write samples to audiofile buffer
        audioFile.samples[channel][wavSample] = (float)outBuffer[i];
        if (channel == 1) wavSample++;
    }

    if (vocoderEnabled && autotuneEnabled) {
        float currentPitch = pitchDetector->getPitch();
        float ratio = targetFrequency / currentPitch;
        vocoder->setBetaFactor(ratio);
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
        bool continuousRec = false;
        if (looper->isRecording()) {
            continuousRec = true;
            looper->stopRec();
        }

        int chNum = tosc_getNextInt32(msg);
        int varNum = tosc_getNextInt32(msg);
        if (continuousRec
                || looper->getChannelVariation(chNum) == varNum
                || looper->isSlotEmpty(chNum, varNum)) {
            looper->setActiveChannel(chNum);
            looper->setActiveVariation(varNum);
        }
        looper->setChannelVariation(chNum, varNum);

        if (continuousRec) {
            looper->startRec();
            osc->sendClipSummary(looper->getClipSummary().dump());
        }
        osc->sendActive(looper->getActiveChannel(), looper->getActiveVariation());
        osc->sendChannelSummary(looper->getChannelSummary().dump());
    }
    else if (address == "loopergroupvariation") {
        int varNum = tosc_getNextInt32(msg);
        looper->setAllVariations(varNum);
        osc->sendActive(looper->getActiveChannel(), looper->getActiveVariation());
        osc->sendChannelSummary(looper->getChannelSummary().dump());
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
        osc->sendChannelSummary(looper->getChannelSummary().dump());
    }
    else if (address == "getwidgets") {
        osc->sendJson(looper->getWidgetJSON().c_str());
    }
    else if (address == "updatezone") {
        string zoneString = tosc_getNextString(msg);
        float zoneValue = tosc_getNextFloat(msg);
        long zonePointer = stol(zoneString);
        looper->storeWidgetAutomation(zonePointer, zoneValue);
        float* zone = (float*)zonePointer;
        *zone = zoneValue;
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
    else if (address == "/theremin") {
        float imuX = tosc_getNextFloat(msg);
        float imuY = tosc_getNextFloat(msg);
        float imuZ = tosc_getNextFloat(msg);
        float frequency = tosc_getNextFloat(msg);
        float pitch = tosc_getNextFloat(msg);
        float fsr1 = tosc_getNextFloat(msg);
        float fsr2 = tosc_getNextFloat(msg);
        float fsr3 = tosc_getNextFloat(msg);

        float thereminValues[Theremin::nInputs];
        int i = 0;
        thereminValues[i++] = fsr1*2;
        thereminValues[i++] = fsr2*2;
        thereminValues[i++] = fsr3*2;
        thereminValues[i++] = frequency;
        thereminValues[i++] = imuX;
        thereminValues[i++] = imuY;
        thereminValues[i++] = imuZ;
        Theremin::setValues(thereminValues);

        if (autotuneEnabled) targetFrequency = frequency;
        else vocoder->setBetaFactor(pitch);
    }
    else if (address == "mastervolume") {
        float volume = tosc_getNextFloat(msg);
        masterVolume = volume;
    }
}

int main() {
    // Audio parameters
    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 128;

    audioFile.setBitDepth(16);
    audioFile.setSampleRate(sampleRate);
    audioFile.setNumChannels(2);

    SFSynth::init(sampleRate * 2, bufferFrames);
    osc = new OSC(oscCallback);
    looper = new Looper(osc, sampleRate);
    vocoder = new Vocoder();
    pitchDetector = new PitchDetector(sampleRate, 2048);
    fileManager = new FileManager();
    audioEngine = new AudioEngine(sampleRate, bufferFrames, &render, RtAudio::LINUX_ALSA);

#ifdef MIDI_ENABLED
    // Initialize MIDI listener
    RtMidiIn *midiIn = new RtMidiIn();
    // Check available ports.
    unsigned int nPorts = midiIn->getPortCount();
    if ( nPorts == 0 ) {
        cout << "No ports available!\n";
    }
    else {
        cout << nPorts << " MIDI port(s) available\n";
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
    cout << "\nRunning ... press <enter> to quit.\n";
    cin.get(input);
    audioEngine->shutDown();
    osc->closeSocket();

    // Save wav file
    fileManager->writeWaveFile(&audioFile, "wav/%c.wav");

    return 0;
}