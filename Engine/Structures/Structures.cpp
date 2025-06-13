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

std::string Engine::ClipBase::generateUniqueColor(const std::string& clipID) {
    std::hash<std::string> hasher;
    size_t hash = hasher(clipID);
    std::mt19937 gen(static_cast<unsigned int>(hash));
    std::uniform_int_distribution<> hueDist(0, 360); // Оттенок в градусах
    std::uniform_int_distribution<> satDist(50, 90); // Насыщенность (50-90%)
    std::uniform_int_distribution<> valDist(70, 95); // Яркость (70-95%)

    int hue = hueDist(gen);
    int saturation = satDist(gen); // Достаточная насыщенность
    int value = valDist(gen);     // Достаточная яркость

    // Преобразование HSV в RGB (используем простой алгоритм)
    float h = hue / 60.0f;
    int i = static_cast<int>(h);
    float f = h - i;
    float p = value / 100.0f * (1.0f - saturation / 100.0f);
    float q = value / 100.0f * (1.0f - f * saturation / 100.0f);
    float t = value / 100.0f * (1.0f - (1.0f - f) * saturation / 100.0f);

    float r, g, b;
    switch (i % 6) {
    case 0: r = value / 100.0f; g = t; b = p; break;
    case 1: r = q; g = value / 100.0f; b = p; break;
    case 2: r = p; g = value / 100.0f; b = t; break;
    case 3: r = p; g = q; b = value / 100.0f; break;
    case 4: r = t; g = p; b = value / 100.0f; break;
    case 5: r = value / 100.0f; g = p; b = q; break;
    }

    // Преобразование в шестнадцатеричный формат
    std::stringstream ss;
    ss << "#" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(r * 255)
        << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(g * 255)
        << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b * 255);
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
    this->startBeats = startBeats; // Явно устанавливаем поле объекта
    startTime = master->startTime;
    duration = master->duration;
    durationBeats = master->durationBeats;
    gain = master->gain;
    muted = master->muted;
    masterClipID = master->clipID;
    color = master->color;
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