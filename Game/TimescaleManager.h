#pragma once
#include "DxLib.h"
#include <vector>

namespace Game {

class TimescaleManager {
public:
    TimescaleManager();
    ~TimescaleManager() = default;

    void SetTargetTimescale(float target);
    float GetCurrentTimescale() const { return currentTimescale_; }
    float GetRawTimescale() const { return targetTimescale_; }
    void Update(float dt);
    void SetTimescaleImmediate(float scale);
    bool IsSlowActive() const { return targetTimescale_ < 0.99f; }

private:
    float currentTimescale_ = 1.0f;
    float targetTimescale_ = 1.0f;
    float transitionSpeed_ = 5.0f;
};

} // namespace Game
