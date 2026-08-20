#pragma once
#include "DxLib.h"
#include <vector>

namespace Game {

// ==========================================
// Timescale Manager - Controls time dilation for slow-motion
// ==========================================
class TimescaleManager {
public:
    TimescaleManager();
    ~TimescaleManager() = default;

    // Set target timescale (1.0 = normal, 0.2 = slow-mo)
    void SetTargetTimescale(float target);

    // Get current timescale with interpolation
    float GetCurrentTimescale() const { return currentTimescale_; }

    // Get raw timescale (not interpolated)
    float GetRawTimescale() const { return targetTimescale_; }

    // Update interpolation toward target
    void Update(float dt);

    // Immediately set timescale without interpolation
    void SetTimescaleImmediate(float scale);

    // Check if currently in slow-mo
    bool IsSlowActive() const { return targetTimescale_ < 0.99f; }

private:
    float currentTimescale_ = 1.0f;  // Interpolated value used in game
    float targetTimescale_ = 1.0f;   // Target value to reach
    float transitionSpeed_ = 5.0f;   // Speed of interpolation
};

} // namespace Game
