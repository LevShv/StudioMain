#include "engine.h"
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <log.h>

#pragma region Clipbase implementation

std::string Engine::ClipBase::generateClipID() {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9999);
    std::stringstream ss;
    ss << std::hex << now << dis(gen);
    return ss.str();
}

bool Engine::ClipBase::isActive(double time) const {
    return time >= startTime && time < startTime + duration;
}

bool Engine::ClipBase::isActiveInRange(double startTime, double endTime) const {
    const double epsilon = 0.0001;
    return (this->startTime <= endTime + epsilon) &&
        (this->startTime + this->duration >= startTime - epsilon);
}

#pragma endregion

#pragma region Cloneclip implementation

Engine::CloneClip::CloneClip(ClipBase* master, double startBeats) {
    masterClip = master;
    this->startBeats = startBeats; // явно устанавливаем поле объекта
    startTime = master->startTime;
    duration = master->duration;
    durationBeats = master->durationBeats;
    gain = master->gain;
    muted = master->muted;
    masterClipID = master->clipID;
    LOG_SUCCESS("CloneClip constructed: startBeats:" << this->startBeats);
}

bool Engine::CloneClip::isActive(double time) const {
    return time >= startTime && time < startTime + duration;
}

bool Engine::CloneClip::isActiveInRange(double startTime, double endTime) const {
    const double epsilon = 0.0001;
    return (this->startTime <= endTime + epsilon) &&
        (this->startTime + this->duration >= startTime - epsilon);
}

#pragma endregion

#pragma region PluginInstance implementation

Engine::PluginInstance::~PluginInstance() {
    if (editor) delete editor;
}

#pragma endregion