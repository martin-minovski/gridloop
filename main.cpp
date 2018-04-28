#define TSF_IMPLEMENTATION

#include <iostream>
#include <map>
#include "rtaudio/RtAudio.h"
#include "stk/RtMidi.h"
#include "tsf.h"
#include "Looper.h"
#include "Vocoder.h"
#include "OSC.h"
using namespace std;

tsf* TinySoundFont;
float tsfBuffer[1024];


int sustained[127];
bool sustainOn = false;
int tsfPreset = 23;

double globalGain = 0.5;

Looper looper;
Vocoder vocoder;
OSC osc;

void probeDevices(RtAudio &audio) {
    int devices = audio.getDeviceCount();
    RtAudio::DeviceInfo info;
    for (unsigned int i=0; i<devices; i++) {
        info = audio.getDeviceInfo(i);

        std::cout << "\nDevice Name = " << info.name << '\n';
        if ( info.probed == false )
            std::cout << "Probe Status = UNsuccessful\n";
        else {
            std::cout << "Probe Status = Successful\n";
            std::cout << "Output Channels = " << info.outputChannels << '\n';
            std::cout << "Input Channels = " << info.inputChannels << '\n';
            std::cout << "Duplex Channels = " << info.duplexChannels << '\n';
            if ( info.isDefaultOutput ) std::cout << "This is the default output device.\n";
            else std::cout << "This is NOT the default output device.\n";
            if ( info.isDefaultInput ) std::cout << "This is the default input device.\n";
            else std::cout << "This is NOT the default input device.\n";
            if ( info.nativeFormats == 0 )
                std::cout << "No natively supported data formats(?)!";
            else {
                std::cout << "Natively supported data formats:\n";
                if ( info.nativeFormats & RTAUDIO_SINT8 )
                    std::cout << "  8-bit int\n";
                if ( info.nativeFormats & RTAUDIO_SINT16 )
                    std::cout << "  16-bit int\n";
                if ( info.nativeFormats & RTAUDIO_SINT24 )
                    std::cout << "  24-bit int\n";
                if ( info.nativeFormats & RTAUDIO_SINT32 )
                    std::cout << "  32-bit int\n";
                if ( info.nativeFormats & RTAUDIO_FLOAT32 )
                    std::cout << "  32-bit float\n";
                if ( info.nativeFormats & RTAUDIO_FLOAT64 )
                    std::cout << "  64-bit float\n";
            }
            if ( info.sampleRates.size() < 1 )
                std::cout << "No supported sample rates found!";
            else {
                std::cout << "Supported sample rates = ";
                for (unsigned int j=0; j<info.sampleRates.size(); j++)
                    std::cout << info.sampleRates[j] << " ";
            }
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

void printCurrentAudioDriver(RtAudio& audio) {
    // Create an api map.
    std::map<int, std::string> apiMap;
    apiMap[RtAudio::MACOSX_CORE] = "OS-X Core Audio";
    apiMap[RtAudio::WINDOWS_ASIO] = "Windows ASIO";
    apiMap[RtAudio::WINDOWS_DS] = "Windows Direct Sound";
    apiMap[RtAudio::WINDOWS_WASAPI] = "Windows WASAPI";
    apiMap[RtAudio::UNIX_JACK] = "Jack Client";
    apiMap[RtAudio::LINUX_ALSA] = "Linux ALSA";
    apiMap[RtAudio::LINUX_PULSE] = "Linux PulseAudio";
    apiMap[RtAudio::LINUX_OSS] = "Linux OSS";
    apiMap[RtAudio::RTAUDIO_DUMMY] = "RtAudio Dummy";

    std::cout << "\nCurrent API: " << apiMap[ audio.getCurrentApi() ] << std::endl;
}

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


unsigned int bufferFrames = 256; // 256 sample frames

int render(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames,
           double streamTime, RtAudioStreamStatus status, void *userData) {

    int nChannels = 2;
    double *inBuffer = (double *) inputBuffer;
    double *outBuffer = (double *) outputBuffer;

    tsf_render_float(TinySoundFont, tsfBuffer, nChannels * nBufferFrames, 0);

    if (status) {
        std::cout << "Stream underflow detected!" << std::endl;
    }

    looper.render(inBuffer, outBuffer, nBufferFrames);

    int totalFrames = nBufferFrames * nChannels;
    for (int i = 0; i < totalFrames; i++) {
        float tsfSample = tsfBuffer[i];
        double inputSample = inBuffer[i];

        float fftSample = 0;
        if (i % 2 == 0) {
            fftSample = vocoder.processSample(tsfSample);
        }
        else {
            fftSample = tsfSample;
        }

        outBuffer[i] = fftSample;
    }

    osc.oscListen();

    return 0;
}

void processOscMsg(tosc_message* msg) {
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

    osc.setCallback(processOscMsg);

    // Load TSF
    unsigned int sampleRate = 44100;
    unsigned int gainDb = 0;
    TinySoundFont = tsf_load_filename("omega.sf2");
    tsf_set_output(TinySoundFont, TSF_STEREO_UNWEAVED, sampleRate, gainDb);
    for (int i = 0; i < 128; i++) {
        sustained[i] = 0;
    }

    // Initialize realtime audio thread

    RtAudio dac = RtAudio(RtAudio::UNIX_JACK);
    if ( dac.getDeviceCount() < 1 ) {
        std::cout << "\nNo audio devices found!\n";
        exit( 0 );
    }
    cout << endl << dac.getDeviceCount() << " devices found" << endl;

    printCurrentAudioDriver(dac);

//    probeDevices(dac);

    RtAudio::StreamParameters parametersOut;
    parametersOut.deviceId = dac.getDefaultOutputDevice();
    parametersOut.nChannels = 2;
    parametersOut.firstChannel = 0;

    RtAudio::StreamParameters parametersIn;
    parametersIn.deviceId = dac.getDefaultInputDevice();
    parametersIn.nChannels = 2;
    parametersIn.firstChannel = 0;

    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_SCHEDULE_REALTIME;
//    options.flags = RTAUDIO_NONINTERLEAVED;


    double data[2];
    try {
        dac.openStream( &parametersOut, &parametersIn, RTAUDIO_FLOAT64,
                        sampleRate, &bufferFrames, &render, (void *)&data, &options );
        dac.startStream();
    }
    catch ( RtAudioError& e ) {
        e.printMessage();
        exit( 0 );
    }

    // Initialize looper
    looper = Looper();

    // Initialize vocoder
    vocoder = Vocoder();

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
    std::cout << "\nPlaying ... press <enter> to quit.\n";
    std::cin.get( input );
    try {
        // Stop the stream
        dac.stopStream();
    }
    catch (RtAudioError& e) {
        e.printMessage();
    }
    if ( dac.isStreamOpen() ) dac.closeStream();

    osc.closeSocket();

    return 0;
}