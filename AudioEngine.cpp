//
// LoopGrid by Martin Minovski, 2018
//

#include "AudioEngine.h"
using namespace std;

AudioEngine::AudioEngine(unsigned int sampleRate, unsigned int bufferFrames, RtAudioCallback render, RtAudio::Api api) {
    
    dac = new RtAudio(api);
    if ( dac->getDeviceCount() < 1 ) {
        std::cout << "\nNo audio devices found!\n";
        exit( 0 );
    }
    cout << endl << dac->getDeviceCount() << " audio devices found" << endl;

    printCurrentAudioDriver();

//    probeDevices();

    RtAudio::StreamParameters parametersOut;
    parametersOut.deviceId = dac->getDefaultOutputDevice();
    parametersOut.nChannels = 2;
    parametersOut.firstChannel = 0;

    RtAudio::StreamParameters parametersIn;
    parametersIn.deviceId = dac->getDefaultInputDevice();
    parametersIn.nChannels = 2;
    parametersIn.firstChannel = 0;

    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_SCHEDULE_REALTIME;
//    options.flags = RTAUDIO_NONINTERLEAVED;


    double data[2];
    try {
        dac->openStream( &parametersOut, &parametersIn, RTAUDIO_FLOAT64,
                        sampleRate, &bufferFrames, render, (void *)&data, &options );
        dac->startStream();
    }
    catch ( RtAudioError& e ) {
        e.printMessage();
        exit( 0 );
    }
}

void AudioEngine::shutDown() {
    try {
        // Stop the stream
        dac->stopStream();
    }
    catch (RtAudioError& e) {
        e.printMessage();
    }
    if ( dac->isStreamOpen() ) dac->closeStream();
}

void AudioEngine::probeDevices() {
    int devices = dac->getDeviceCount();
    RtAudio::DeviceInfo info;
    for (unsigned int i=0; i<devices; i++) {
        info = dac->getDeviceInfo(i);

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

void AudioEngine::printCurrentAudioDriver() {
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

    std::cout << "Current audio API: " << apiMap[ dac->getCurrentApi() ] << std::endl;
}