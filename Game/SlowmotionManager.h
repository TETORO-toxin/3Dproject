#pragma once
#include "DxLib.h"
#include "TimescaleManager.h"
#include "JustTimingSystem.h"
#include "ComboSystem.h"
#include "Coin.h"
#include <vector>

namespace Game {

// ==========================================
// SlowmotionManager - Orchestrates slow-motion mechanics
// ==========================================
class SlowmotionManager {
public:
    SlowmotionManager();
    ~SlowmotionManager() = default;

    // Activate slow-motion
    void ActivateSlow();

    // Deactivate slow-motion
    void DeactivateSlow();

    // Check if slow-motion is active
    bool IsSlowActive() const { return timescaleManager_.IsSlowActive(); }

    // Get current timescale to apply to delta time
    float GetCurrentTimescale() const { return timescaleManager_.GetCurrentTimescale(); }

    // Mark coin as entering just window
    void OnCoinHitStart(int coinIndex) { justTimingSystem_.StartJustWindow(coinIndex); }

    // Check if current frame is just
    bool IsJustFrame(int coinIndex) const { return justTimingSystem_.IsJustFrame(coinIndex); }

    // Record a hit
    void RecordHit(const Coin& coin, bool isJust);

    // Get combo info
    int GetCombo() const { return comboSystem_.GetCombo(); }
    float GetComboMultiplier() const { return comboSystem_.GetComboMultiplier(); }

    // Update all systems
    void Update(float dt);

    // Reset all systems
    void Reset();

private:
    TimescaleManager timescaleManager_;
    JustTimingSystem justTimingSystem_;
    ComboSystem comboSystem_;
};

} // namespace Game
