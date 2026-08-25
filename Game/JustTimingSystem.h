#pragma once
#include "DxLib.h"
#include "Coin.h"
#include <vector>
#include "GameParams.h"

namespace Game {

// ==========================================
// JustTiming System - Detects "just" hits for bonus damage
// ==========================================
class JustTimingSystem {
public:
    JustTimingSystem();
    ~JustTimingSystem() = default;

    // Mark that we're in "just window" for a coin
    void StartJustWindow(int coinIndex);

    // Check if current frame is within "just" window
    bool IsJustFrame(int coinIndex) const;

    // Get just hit bonus multiplier
    float GetJustMultiplier() const { return Params::JUST_HIT_DAMAGE_MULT; }

    // Update just windows (decrement frame counters)
    void Update();

    // Reset all just windows
    void Clear();

private:
    struct JustWindow {
        int coinIndex = -1;
        int framesRemaining = 0;
    };

    std::vector<JustWindow> justWindows_;
};

} // namespace Game
