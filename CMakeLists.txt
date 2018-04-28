cmake_minimum_required(VERSION 3.9)
project(rtpiano)
set(CMAKE_CXX_STANDARD 11)
include_directories(/usr/local/include/rtaudio)
include_directories(/usr/local/include/stk)
file(GLOB rtpiano_SRC
    "*.h"
    "*.cpp"
)
add_executable(rtpiano ${rtpiano_SRC} Looper.cpp Looper.h tinyosc.c tinyosc.h kiss_fft.c kiss_fft.h _kiss_fft_guts.h Vocoder.cpp Vocoder.h OSC.cpp OSC.h AudioEngine.cpp AudioEngine.h SFSynth.cpp SFSynth.h)
target_link_libraries(rtpiano rtaudio)
target_link_libraries(rtpiano stk)