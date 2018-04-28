#define TSF_IMPLEMENTATION

#include <iostream>
#include "rtaudio/RtAudio.h"
#include "stk/RtMidi.h"
#include "tsf.h"
#include "Looper.h"
#include "Vocoder.h"
#include "OSC.h"
#include "AudioEngine.h"
using namespace std;

tsf* TinySoundFont;
float tsfBuffer[1024];


int sustained[127];
bool sustainOn = false;
int tsfPreset = 23;

double globalGain = 0.5;

Looper* looper;
Vocoder* vocoder;
OSC* osc;
AudioEngine* audioEngine;


void midiCallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{

    unsigned int nBytes = message->size();
    for ( unsigned int i=0; i<nBytes; i++ )
        std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
    if ( nBytes > 0 )
        std::cout << "stamp = " << deltatime << std::endl;


    if (message->at(0) == 144) {
        int pitch = message->at(1);
        if (message->at(2) == 0) {
            if (sustainOn) sustained[pitch] = 1;
            else tsf_note_off(TinySoundFont, tsfPreset, pitch);
        }
        else {
            if (pitch == 107) {
                tsfPreset--;
            }
            else if (pitch == 108) {
                tsfPreset++;
            }
            else {
                tsf_note_off(TinySoundFont, tsfPreset, pitch);
                tsf_note_on(TinySoundFont, tsfPreset, pitch, (float)message->at(2)/128);
                sustained[pitch] = false;
            }

        }

    }

//    if (message->at(0) == 176) {
//        sustainOn = (message->at(2) > 0);
//        if (!sustainOn) {
//            for (int i = 0; i < 128; i++) {
//                if (sustained[i]) tsf_note_off(TinySoundFont, tsfPreset, i);
//            }
//        }
//    }
}


int render(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void *userData) {

    int nChannels = 2;
    double *inBuffer = (double *) inputBuffer;
    double *outBuffer = (double *) outputBuffer;

    tsf_render_float(TinySoundFont, tsfBuffer, nChannels * nBufferFrames, 0);

    if (status) {
        std::cout << "Stream underflow detected!" << std::endl;
    }

    looper->render(inBuffer, outBuffer, nBufferFrames);

    int totalFrames = nBufferFrames * nChannels;
    for (int i = 0; i < totalFrames; i++) {
        float tsfSample = tsfBuffer[i];
        double inputSample = inBuffer[i];

        float fftSample = 0;
        if (i % 2 == 0) {
            fftSample = vocoder->processSample(tsfSample);
        }
        else {
            fftSample = tsfSample;
        }

        outBuffer[i] = fftSample;
    }

    osc->oscListen();

    return 0;
}

void oscCallback(tosc_message* msg) {
//    tosc_printMessage(msg);

    string address = tosc_getAddress(msg);

    if (address == "slider") {
        int id = tosc_getNextInt32(msg);
        double gain = tosc_getNextFloat(msg);
        globalGain = gain;
    }
    if (address == "piano") {
        int pitch = tosc_getNextInt32(msg);
        int state = tosc_getNextInt32(msg);
        if (state == 1) {
            tsf_note_off(TinySoundFont, tsfPreset, pitch);
            tsf_note_on(TinySoundFont, tsfPreset, pitch, 0.8f);
        }
        else {
            tsf_note_off(TinySoundFont, tsfPreset, pitch);
        }
    }
}

int main() {

    // Audio parameters
    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 256;

    // Load TSF
    TinySoundFont = tsf_load_filename("omega.sf2");
    tsf_set_output(TinySoundFont, TSF_STEREO_UNWEAVED, sampleRate, 0);
    for (int i = 0; i < 128; i++) {
        sustained[i] = 0;
    }

    looper = new Looper();
    vocoder = new Vocoder();
    osc = new OSC(oscCallback);

    // Initialize audio
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