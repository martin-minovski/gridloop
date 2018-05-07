//
// Created by martin on 5/7/18.
//

#ifndef RTPIANO_FILEMANAGER_H
#define RTPIANO_FILEMANAGER_H

#include <string>
#include <fstream>
using namespace std;

class FileManager {
public:
    string getFaustCode(int channel);
    void writeFaustCode(int channel, string code);
};


#endif //RTPIANO_FILEMANAGER_H
