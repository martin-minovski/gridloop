//
// LoopGrid by Martin Minovski, 2018
//

#include "FileManager.h"

string FileManager::getFaustCode(int channel) {
    string path = "dsp/" + to_string(channel)+ ".dsp";
    ifstream ifs(path);
    string content( (std::istreambuf_iterator<char>(ifs)),
                         (std::istreambuf_iterator<char>()));
    return content;
}
void FileManager::writeFaustCode(int channel, string code) {
    string path = "dsp/" + to_string(channel)+ ".dsp";
    ofstream out(path);
    out << code;
    out.close();
}
void FileManager::writeWaveFile(AudioFile<float> *audioFile, string path) {
    time_t rawtime;
    struct tm * timeinfo;
    char dateBuffer [80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(dateBuffer, 80, path.c_str(), timeinfo);
    audioFile->save(dateBuffer);
}