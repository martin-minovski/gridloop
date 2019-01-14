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
//#define MIDI_DEBUG
//#define USING_ALSA

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
float masterVolume = 1.2f;
float thereminVolume = 1.0f;
float vocoderUpperThreshold = 20000;
bool vocoderMixOriginal = false;
bool sustainPedalAsLooper = false;
bool lineIn = true;
float sfSynthVolume = 3.0f;
RtMidiOut *midiOut;
double midiNumb = 40;
double inputVolume = 0.20f;
float inputDryWet = 0;
int transpose = 0;
bool shiftHeld = false;
bool recHeld = false;
int instrPage = 0;

void setLooperChannel(int chNum, int varNum);
void hitEffectPad(int ch, int var);
void clearChVar(int ch, int var);

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
void instrCtrl(int key, int pageSize) {
    int pageOffset = instrPage * pageSize;
    int bank = (pageOffset + key) / 128;
    int preset = (pageOffset + key) % 128;
    SFSynth::setPreset(bank, preset);
}

void midiCallback(double deltatime, vector<unsigned char> *message, void *userData)
{
    auto msg = new smf::MidiMessage(*message);

#ifdef MIDI_DEBUG
    cout << "MSG ---\t" <<
    "CH " << msg->getChannel() <<
        (msg->isNoteOn() ? "\tNOTE ON " : "") <<
        (msg->isNoteOff() ? "\tNOTE OFF " : "") <<
        (msg->isController() ? "\tCC " : "") <<
        (msg->isPitchbend() ? (" \tPB ") : "") <<
        (msg->isMetaMessage() || msg->isMeta() ? "\tMETA " : "") <<
        "\t# " << msg->getP1() <<
        "\t## " << msg->getP2() <<
        "\t### " << msg->isPitchbend();
    if (msg->isPitchbend()) {
        cout<< "\t#PB " << ((msg->getP2() - 64) * 128 + msg->getP1());
    }
    cout<<endl;
#endif
    if (msg->isPitchbend()) {
        int value = ((msg->getP2() - 64) * 128 + msg->getP1()) + 8192;
        SFSynth::setPitchWheel(value);
    }
    if (msg->isController()) {
        int id = message->at(1);
        int value = message->at(2);
        if (id == 64) {
            if (!sustainPedalAsLooper) {
                if (message->at(2) > 0) {
                    looper->faustSustain(true);
                    SFSynth::sustainOn();
                }
                else {
                    looper->faustSustain(false);
                    SFSynth::sustainOff();
                }
            }
            else {
                if (message->at(2) == 127) {
                    if (looper->isRecording()) {
                        looper->stopRec();
                        osc->sendClipSummary(looper->getClipSummary().dump());
                    } else {
                        looper->startRec();
                    }
                }
            }
        }
        else if (id == 117 && message->at(2) == 0) {
            shiftHeld = false;
        }
        else if (id == 113 && message->at(2) == 0) {
            recHeld = false;
        }
        else if (message->at(2) != 0) {
            // Pad on
            if (id == 113) {
                if (!shiftHeld) {
                    // Record
                    recHeld = true;
                    if (looper->isRecording()) {
                        looper->stopRec();
                        looperListening = false;
                    }
                    else {
                        if (looperListening) {
                            looperListening = false;
                            looper->startRec();
                        }
                        else looperListening = true;
                    }
                    osc->sendClipSummary(looper->getClipSummary().dump());
                }
                else {
                    looper->undo();
                    osc->sendClipSummary(looper->getClipSummary().dump());
                }
            }
            else if (id == 117) {
                if (!recHeld) {
                    shiftHeld = true;
                }
                else {
                    looper->clearAll();
                    osc->sendClipSummary(looper->getClipSummary().dump());
                }
            }
            else if (id == 20) {
                masterVolume = float(value) / 64;
            }
            else if (id == 21) {
                sfSynthVolume = float(value) / 20;
                // vocoder->setBetaFactor(0.95f + ((float)value / 1280.0f));
            }
            else if (id == 22) {
                looper->FaustCC(id, value);
                // SFSynth::setPreset(0, value);
            }
            else if (id == 23) {
                midiNumb = (((double)value / 128) - 0.5) * 200;
                cout << "New deviation: " << midiNumb << endl;
            }
            else if (id == 24) {
                inputVolume = float(value) / 127;
            }
            else if (id == 25) {
                inputDryWet = float(value) / 127;
            }
            else if (id == 26) {
                looper->FaustCC(id, value);
            }
            else if (id == 27) {
                looper->FaustCC(id, value);
            }
            else if (msg->getChannel() == 0) {
                if (id == 110) {
                    if (!shiftHeld) hitEffectPad(0, 1);
                    else clearChVar(0, 1);
                }
                else if (id == 111) {
                    if (!shiftHeld) hitEffectPad(1, 1);
                    else clearChVar(1, 1);
                }
                else if (id == 112) {
                    if (!shiftHeld) hitEffectPad(2, 1);
                    else clearChVar(2, 1);
                }
                else if (id == 114) {
                    if (!shiftHeld) hitEffectPad(0, 2);
                    else clearChVar(0, 2);
                }
                else if (id == 115) {
                    if (!shiftHeld) hitEffectPad(1, 2);
                    else clearChVar(1, 2);
                }
                else if (id == 116) {
                    if (!shiftHeld) hitEffectPad(2, 2);
                    else clearChVar(2, 2);
                }
            }
            else if (msg->getChannel() == 1) {
                if (id == 110) {}
                else if (id == 111) {}
                else if (id == 112) {}
                else if (id == 114) {}
                else if (id == 115) {}
                else if (id == 116) {}
            }
        }
        else {
            // Pad note off
        }
    }
    else if (msg->isNoteOn()) {
        int pitch = message->at(1);
        int velocity = message->at(2);
        if (shiftHeld) {
            int absolutePitch = pitch % 12;
            int instrOct =(pitch-24) / 12;
            if (absolutePitch == 8) {
                transpose = 0;
                SFSynth::panic();
            }
            else if (absolutePitch == 6) {
                transpose--;
                SFSynth::panic();
            }
            else if (absolutePitch == 10) {
                transpose++;
                SFSynth::panic();
            }
            else if (absolutePitch == 1) {
                if (instrPage > 0) instrPage--;
            }
            else if (absolutePitch == 3) {
                instrPage++;
            }
            else if (pitch >= 24 && pitch <= 96) {
                int octSize = 7;
                int pageSize = 36;
                if (absolutePitch == 0) {
                    instrCtrl(instrOct*octSize, pageSize);
                }
                else if (absolutePitch == 2) {
                    instrCtrl(instrOct*octSize+1, pageSize);
                }
                else if (absolutePitch == 4) {
                    instrCtrl(instrOct*octSize+2, pageSize);
                }
                else if (absolutePitch == 5) {
                    instrCtrl(instrOct*octSize+3, pageSize);
                }
                else if (absolutePitch == 7) {
                    instrCtrl(instrOct*octSize+4, pageSize);
                }
                else if (absolutePitch == 9) {
                    instrCtrl(instrOct*octSize+5, pageSize);
                }
                else if (absolutePitch == 11) {
                    instrCtrl(instrOct*octSize+6, pageSize);
                }
            }
        }
        else {
            // Actual note on
            if (looperListening) {
                looper->startRec();
                looperListening = false;
            }
            SFSynth::noteOn(pitch + transpose, convertMidiValue(velocity, midiNumb));
            looper->faustNoteOn(pitch + transpose, velocity);
            return;
        }
//        midiOut->sendMessage(message);
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
    else if (msg->isNoteOff()) {
        int pitch = message->at(1);
        SFSynth::noteOff(pitch + transpose);
        looper->faustNoteOff(pitch + transpose);
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
        tsfSample *= sfSynthVolume;

        float inputSample = (float) inBuffer[i];
        if (!lineIn) inputSample = 0;
        else inputSample *= inputVolume;


        // Process DSP
        float processedSample = looper->process(
                inputDryWet*inputSample +
                (1.0f-inputDryWet)*tsfSample
        );
        float result = processedSample
                + inputDryWet*tsfSample
                + (1.0f-inputDryWet)*inputSample;

        if (vocoderEnabled) {
            // Apply only on left channel
            if (channel == 0) {
                fftSample = vocoder->processSample(result);
                pitchDetector->process(result);
            }

            if (vocoderMixOriginal) result += (1.3*fftSample);
            else inputSample = fftSample;
        }

        // Write samples to audiofile buffer
        audioFile.samples[channel][wavSample] = result;
        if (channel == 1) wavSample++;

        // Submit
        outBuffer[i] = result * masterVolume;
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

void setLooperChannel(int chNum, int varNum) {
    bool continuousRec = false;
    if (looper->isRecording()) {
        continuousRec = true;
        looper->stopRec();
    }

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
void clearChVar(int ch, int var) {
    looper->clearChannel(ch, var);
    osc->sendClipSummary(looper->getClipSummary().dump());
}
void hitEffectPad(int ch, int var) {
    setLooperChannel(ch, var);
//    looper->setAllVariations(var);
    osc->sendActive(looper->getActiveChannel(), looper->getActiveVariation());
    osc->sendChannelSummary(looper->getChannelSummary().dump());
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
        setLooperChannel(chNum, varNum);
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
        clearChVar(chNum, varNum);
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

#ifdef USING_ALSA
    audioEngine = new AudioEngine(sampleRate, bufferFrames, &render, RtAudio::LINUX_ALSA);
#endif
#ifndef USING_ALSA
    audioEngine = new AudioEngine(sampleRate, bufferFrames, &render, RtAudio::UNIX_JACK);
#endif

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
#ifdef USING_ALSA
        midiIn->openPort(1);
#endif
#ifndef USING_ALSA
        midiIn->openPort(0);
#endif
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

    setLooperChannel(0, 1);

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