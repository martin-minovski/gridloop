//
// Created by martin on 5/7/18.
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