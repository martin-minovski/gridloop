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
#define USING_ALSA

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
void noteOn(int pitch, int velocity) {
    if (looperListening) {
        looper->startRec();
        looperListening = false;
    }
    SFSynth::noteOn(pitch + transpose, convertMidiValue(velocity, midiNumb));
    looper->faustNoteOn(pitch + transpose, velocity);
}
void noteOff(int pitch) {
    SFSynth::noteOff(pitch + transpose);
    looper->faustNoteOff(pitch + transpose);
}
void pushRec() {
    if (!shiftHeld) {
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
void pushShift() {
    // Shift always stops loper from listening
    if (looperListening) looperListening = false;
    if (!recHeld) {
        shiftHeld = true;
    }
    else {
        looper->clearAll();
        osc->sendClipSummary(looper->getClipSummary().dump());
    }
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
        else if (msg->getChannel() > 0 && message->at(1) >= 110 && message->at(1) <= 117) {
            int drumPitch = 35 + (message->at(1)-110)*2 + (msg->getChannel()-1)*3;
            if (message->at(2) > 0) {
                if (id == 113) pushRec();
                else {
                    noteOn(drumPitch, message->at(2));
                    cout<< "Drum " << drumPitch << endl;
                    if (id == 117) pushShift();
                }
            }
            else {
//                noteOff(drumPitch);
            }
        }
        else if (id >= 20 && id <= 27 && shiftHeld) {
            // All shifted knobs go Faust CC
            looper->FaustCC(id - 19, message->at(2), looper->getActiveChannel());
        }
        else if (message->at(2) != 0) {
            if (id == 113) {
                pushRec();
            }
            else if (id == 117) {
                pushShift();
            }
            else if (id == 20) {
                masterVolume = float(value) / 40;
            }
            else if (id == 21) {
                sfSynthVolume = float(value) / 20;
                // vocoder->setBetaFactor(0.95f + ((float)value / 1280.0f));
            }
            else if (id == 22 || id == 26 || id == 27 || id == 16 || id == 17) {
                std::vector<long> zonesToAutomate = looper->FaustCC(id, value, looper->getActiveChannel());
                auto zoneIt = std::begin(zonesToAutomate);
                while (zoneIt != std::end(zonesToAutomate)) {
                    auto zone = *zoneIt;
                    looper->storeWidgetAutomation(zone, (*((float*)zone)));
                    zoneIt++;
                }
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
    }
    else if (msg->isNoteOn()) {
        int pitch = message->at(1);
        int velocity = message->at(2);
        if (shiftHeld) {
            int absolutePitch = pitch % 12;
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
                SFSynth::prevInstrument();
            }
            else if (absolutePitch == 3) {
                SFSynth::nextInstrument();
            }
            // Basses 1
            else if (pitch == 24) {
                SFSynth::setPreset(0, 32);
            }
            else if (pitch == 26) {
                SFSynth::setPreset(0, 33);
            }
            else if (pitch == 28) {
                SFSynth::setPreset(0, 34);
            }
            else if (pitch == 29) {
                SFSynth::setPreset(0, 35);
            }
            else if (pitch == 31) {
                SFSynth::setPreset(0, 36);
            }
            else if (pitch == 33) {
                SFSynth::setPreset(0, 37);
            }
            else if (pitch == 35) {
                SFSynth::setPreset(0, 38);
            }
            // Drums
            else if (pitch == 36) {
                SFSynth::setPreset(128, 127);
            }
            else if (pitch == 38) {
                SFSynth::setPreset(128, 24);
            }
            else if (pitch == 40) {
                SFSynth::setPreset(128, 26);
            }
            else if (pitch == 41) {
                SFSynth::setPreset(128, 27);
            }
            else if (pitch == 43) {
                SFSynth::setPreset(128, 40);
            }
            else if (pitch == 45) {
                SFSynth::setPreset(128, 57);
            }
            else if (pitch == 47) {
                SFSynth::setPreset(128, 25);
            }
            // Basses 2
            else if (pitch == 48) {
                SFSynth::setPreset(2, 0);
            }
            else if (pitch == 50) {
                SFSynth::setPreset(3, 0);
            }
            else if (pitch == 52) {
                SFSynth::setPreset(4, 0);
            }
            else if (pitch == 53) {
                SFSynth::setPreset(6, 0);
            }
            else if (pitch == 55) {
                SFSynth::setPreset(8, 0);
            }
            else if (pitch == 57) {
                SFSynth::setPreset(11, 0);
            }
            else if (pitch == 59) {
                SFSynth::setPreset(13, 0);
            }
            // Keys
            else if (pitch == 60) {
                // Yamaha C5
                SFSynth::setPreset(18, 0);
            }
            else if (pitch == 62) {
                SFSynth::setPreset(18, 33);
            }
            else if (pitch == 64) {
                SFSynth::setPreset(18, 81);
            }
            else if (pitch == 65) {
                SFSynth::setPreset(20, 0);
            }
            else if (pitch == 67) {
                SFSynth::setPreset(19, 80);
            }
            else if (pitch == 69) {
                SFSynth::setPreset(19, 81);
            }
            else if (pitch == 71) {
                SFSynth::setPreset(24, 61);
            }
            // Keys 2
            else if (pitch == 72) {
                SFSynth::setPreset(64, 16);
            }
            else if (pitch == 74) {
                SFSynth::setPreset(16, 16);
            }
            else if (pitch == 76) {
                SFSynth::setPreset(16, 18);
            }
            else if (pitch == 77) {
                SFSynth::setPreset(16, 19);
            }
            else if (pitch == 79) {
                SFSynth::setPreset(11, 124);
            }
            else if (pitch == 81) {
                SFSynth::setPreset(0, 89);
            }
            else if (pitch == 83) {
                SFSynth::setPreset(2, 92);
            }
            else if (pitch == 84) {
                SFSynth::setPreset(8, 103);
            }
            else {
                // Whatever
                SFSynth::setPreset(0, pitch / 2);
            }
        }
        else {
            // Actual note on
            noteOn(pitch, velocity);
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
        noteOff(pitch);
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
    SFSynth::setPreset(18, 0);
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