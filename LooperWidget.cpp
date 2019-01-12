//
// LoopGrid by Martin Minovski, 2018
//

#include "LooperWidget.h"

LooperWidget::LooperWidget(int channel) {
    this->looperChannel = channel;
    this->type = strdup("slider");
};

void LooperWidget::setParams(const char* name, float* zone, float min, float max, float step) {
    this->name = strdup(name);
    this->zone = zone;
    this->min = min;
    this->max = max;
    this->step = step;
}
void LooperWidget::setAxis(const char* axis) {
    this->axis = *axis - 'x';
}
void LooperWidget::setId(const int id) {
    this->id = id;
}
int LooperWidget::getId() {
    return this->id;
}
char* LooperWidget::getType() {
    return this->type;
}
void LooperWidget::setType(const char* type) {
    this->type = strdup(type);

    // Add to Theremin array
    if (strcmp(type, "fsr1") == 0) Theremin::inputs[0].push_back(this);
    if (strcmp(type, "fsr2") == 0) Theremin::inputs[1].push_back(this);
    if (strcmp(type, "fsr3") == 0) Theremin::inputs[2].push_back(this);
    if (strcmp(type, "theremin") == 0) Theremin::inputs[3].push_back(this);
    if (strcmp(type, "imux") == 0) Theremin::inputs[4].push_back(this);
    if (strcmp(type, "imuy") == 0) Theremin::inputs[5].push_back(this);
    if (strcmp(type, "imuz") == 0) Theremin::inputs[6].push_back(this);
}
void LooperWidget::printData() {
    cout<<endl;
    cout<<"=== Widget Data ==="<<endl;
    cout<<"Name: "<<name<<endl;
    cout<<"Zone: "<<zone<<endl;
    cout<<"Min: "<<min<<endl;
    cout<<"Max: "<<max<<endl;
    cout<<"Step: "<<step<<endl;
    cout<<"NexusUI type: "<<type<<endl;
    cout<<"NexusUI axis: "<<axis<<endl;
}
json LooperWidget::getJson() {
    std::string nameStr(name);
    std::string typeStr(type);
    json result = {
        {"name", nameStr},
        {"value", *zone},
        {"zone", (long)zone},
        {"min", min},
        {"max", max},
        {"step", step},
        {"axis", axis},
        {"type", typeStr},
        {"looperChannel", looperChannel},
    };
    return result;
}
bool LooperWidget::isSync() {
    const char* syncType = "sync";
    return strcmp(type, syncType) == 0;
}
void LooperWidget::setValue(float value) {
    *zone = value;
}
void LooperWidget::setNormalValue(float value) {
    *zone = min + value * (max - min);
}