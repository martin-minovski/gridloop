//
// LoopGrid by Martin Minovski, 2018
//

#include <iostream>

#include "rtaudio/RtAudio.h"
#include "rtmidi/RtMidi.h"

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
#include "LooperChannel.h"
#include "MidiMessage.h"

#define MIDI_ENABLED
#define MIDI_DEBUG

using namespace std;

Looper* looper;
Vocoder* vocoder;
OSC* osc;
AudioEngine* audioEngine;
FileManager* fileManager;
PitchDetector* pitchDetector;

bool looperListening = false;
bool vocoderEnabled = false;
bool autotuneEnabled = false;
float masterVolume = 1.0f;
float thereminVolume = 1.0f;
float vocoderUpperThreshold = 20000;
bool vocoderMixOriginal = false;
bool sustainPedalAsLooper = false;
bool lineIn = true;
float sfSynthVolume = 1.0f;
RtMidiOut *midiOut;
double midiNumb = 0;
double inputVolume = 1.0f;

int convertMidiValue(int value, double deviation) {
    if (deviation < -100 || deviation > 100) {
        cout<<"Value must be between -100 and 100: "<<deviation;
    }

    double minMidiValue = 0;
    double maxMidiValue = 127;
    double midMidiValue = 63.5;

    // This is our control point for the quadratic bezier curve
    // We want this to be between 0 (min) and 63.5 (max)
    double controlPointX = midMidiValue + ((deviation / 100) * midMidiValue);
    double t = (double)value / maxMidiValue;
    int delta = (int)round((2 * (1 - t) * t * controlPointX) + (t * t * maxMidiValue));
    return (value - delta) + value;
}

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
    if (message->at(0) == 176) {
        int id = message->at(1);
        int value = message->at(2);
        if (id == 20) {
            masterVolume = float(value) / 64;
        }
        if (id == 21) {
            SFSynth::setPreset(0, value);
        }
        if (id == 22) {
            vocoder->setBetaFactor(0.95f + ((float)value / 1280.0f));
        }
        if (id == 23) {
            midiNumb = (((double)value / 128) - 0.5) * 200;
            cout << "New deviation: " << midiNumb << endl;
        }
        if (id == 24) {
            inputVolume = float(value) / 127;
        }
        else {
            looper->FaustCC(id, value);
            cout << "Faust CC " << id << "=" << value << endl;
        }
    }
    else if (message->at(0) == 144) {
        int pitch = message->at(1);
        int velocity = message->at(2);
        SFSynth::noteOn(pitch, convertMidiValue(velocity, midiNumb));
        return;
        midiOut->sendMessage(message);
//        if (message->at(1) == 64) {
//            // Sustain
//
//            if (!sustainPedalAsLooper) {
//                if (message->at(2) > 0) {
//                    looper->faustSustain(1);
//                    SFSynth::sustainOn();
//                }
//                else {
//                    looper->faustSustain(0);
//                    SFSynth::sustainOff();
//                }
//            }
//            else {
//                if (message->at(2) == 127) {
//                    if (looper->isRecording()) {
//                        looper->stopRec();
//                        osc->sendClipSummary(looper->getClipSummary().dump());
//                    } else {
//                        looper->startRec();
//                    }
//                }
//            }
//        }
//        else if (message->at(1) == 7) {
//            // Slider
//            looper->FaustCC(message->at(2));
//        }

    }
    else if (message->at(0) == 128) {
        int pitch = message->at(1);
        SFSynth::noteOff(pitch);
        return;
        midiOut->sendMessage(message);
//        if (message->at(2) == 0) {
//            looper->faustNoteOff(pitch);
//            SFSynth::noteOff(pitch);
//        }
//        else  {
//            int velocity = message->at(2);
//            looper->faustNoteOn(pitch, velocity);
//            SFSynth::noteOn(pitch, velocity);
//
//            if (looperListening) {
//                looper->startRec();
//                looperListening = false;
//            }
//        }
    }
}

void midiCallbackDrum(double deltatime, vector<unsigned char> *message, void *userData) {
    if (message->at(0) == 153) {
        int pitch = message->at(1);
        if (pitch == 22) pitch = 42;
        int velocity = message->at(2);
        SFSynth::drumOn(pitch, velocity);
        cout<<"Pitch: "<<pitch<<", velocity: "<<velocity<<endl;
    }
}

float targetFrequency = 300;

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
        tsfSample *= 0.45;

        // Get left audio input channel only
        float inputSample = (float) inBuffer[i];
        if (!lineIn) inputSample = 0;
        else inputSample *= inputVolume;

        // Mix with SF
        inputSample += tsfSample * sfSynthVolume;

        // Process DSP
        inputSample = looper->process(inputSample);

        if (vocoderEnabled) {
            // Apply only on left channel
            if (channel == 0) {
                fftSample = vocoder->processSample(inputSample);
                pitchDetector->process(inputSample);
            }

            if (vocoderMixOriginal) inputSample += (1.3*fftSample);
            else inputSample = fftSample;
        }

        // Write samples to audiofile buffer
        audioFile.samples[channel][wavSample] = inputSample;
        if (channel == 1) wavSample++;

        // Submit
        outBuffer[i] = inputSample * masterVolume;
    }

    if (vocoderEnabled && autotuneEnabled) {
        float currentPitch = pitchDetector->getPitch();
        float ratio = targetFrequency / currentPitch;
        if (currentPitch < vocoderUpperThreshold) {
            vocoder->setBetaFactor(ratio);
        }
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
//            vocoderUpperThreshold = value;
            cout<<"New vocoder threshold: "<<value<<endl;
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
//            targetFrequency = (float)(pow(2, ((float)(pitch + octaveShift * 12)-69)/12) * 440);
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
        vocoderEnabled = state;
    }
    else if (address == "vocoderinterpolation") {
        bool state = tosc_getNextInt32(msg) == 1;
        vocoder->setLinearInterpolation(state);
    }
    else if (address == "vocoderparabola") {
        bool state = tosc_getNextInt32(msg) == 1;
        vocoder->setParabolaFit(state);
    }
    else if (address == "vocodermix") {
        bool state = tosc_getNextInt32(msg) == 1;
        vocoderMixOriginal = state;
    }
    else if (address == "vocoderautotune") {
        bool state = tosc_getNextInt32(msg) == 1;
        autotuneEnabled = state;
    }
    else if (address == "pedallooper") {
        bool state = tosc_getNextInt32(msg) == 1;
        sustainPedalAsLooper = state;
    }
    else if (address == "linein") {
        bool state = tosc_getNextInt32(msg) == 1;
        lineIn = state;
    }
    else if (address == "sfvolume") {
        float value = tosc_getNextFloat(msg);
        sfSynthVolume = value;
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

        if (autotuneEnabled) targetFrequency = frequency / 2;
        else vocoder->setBetaFactor(pitch);
        thereminVolume = (fsr3) * 3;
    }
    else if (address == "mastervolume") {
        float volume = tosc_getNextFloat(msg);
        masterVolume = volume;
    }
}

int main() {
    // Audio parameters
    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 256;

    audioFile.setBitDepth(16);
    audioFile.setSampleRate(sampleRate);
    audioFile.setNumChannels(2);

    SFSynth::init(sampleRate * 2, bufferFrames);
    osc = new OSC(oscCallback);
    looper = new Looper(osc, sampleRate);
    vocoder = new Vocoder();
    pitchDetector = new PitchDetector(sampleRate, 2048);
    fileManager = new FileManager();
    audioEngine = new AudioEngine(sampleRate, bufferFrames, &render, RtAudio::UNIX_JACK);

#ifdef MIDI_ENABLED
    // Initialize MIDI listener
    RtMidiIn *midiIn = new RtMidiIn();
    // Check available ports.
//    midiIn->openPort(16, "Extra A");
//    midiIn->openPort(17, "Extra B");
//    midiIn->openPort(18, "Extra C");
//    midiIn->openPort(19, "Extra D");
    unsigned int nPortsIn = midiIn->getPortCount();
    unsigned int nPortsOut = midiIn->getPortCount();
    if ( nPortsIn == 0 ) {
        cout << "No input MIDI ports available!\n";
    }
    else {
        cout << nPortsIn << " input MIDI port(s) available\n";
        midiIn->openPort(nPortsIn - 1);
        midiIn->setCallback(&midiCallback);

        // Ignore sysex, timing, or active sensing messages:
        midiIn->ignoreTypes(true, true, true);

//            RtMidiIn *midiInDrum = new RtMidiIn();
//            unsigned int nPortsDrum = midiInDrum->getPortCount();
//            midiInDrum->openPort(nPortsDrum - 2);
//            midiInDrum->setCallback(&midiCallbackDrum);
//            midiInDrum->ignoreTypes(true, true, true);
    }

    if ( nPortsOut == 0 ) {
        cout << "No output MIDI ports available!\n";
    }
    else {
        cout << nPortsOut << " output MIDI port(s) available\n";
//        midiOut->openPort(1);
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