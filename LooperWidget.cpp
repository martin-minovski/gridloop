//
// Created by martin on 5/5/18.
//

#include "LooperWidget.h"

LooperWidget::LooperWidget() {
    this->type = strdup("Test");
    this->axis = 10;
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
void LooperWidget::setType(const char* type) {
    cout<<"====="<<type<<endl;
    this->type = strdup(type);
    cout<<"====="<<this->type<<endl;
}
float* LooperWidget::getZone() {
    return zone;
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