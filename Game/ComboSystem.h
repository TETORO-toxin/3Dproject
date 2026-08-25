#pragma once
#include "DxLib.h"
#include "Coin.h"
#include "TimescaleManager.h"
#include "JustTimingSystem.h"
#include <vector>

namespace Game {

// ==========================================
// Combo System - Tracks consecutive hits
// ==========================================
class ComboSystem {
public:
    ComboSystem();
    ~ComboSystem() = default;

    // Add hit to combo
    void AddHit(const Coin& coin, bool isJust);

    // Get current combo count
    int GetCombo() const { return currentCombo_; }

    // Get damage multiplier based on combo
    float GetComboMultiplier() const;

    // Break combo
    void BreakCombo();

    // Get just count (for SFX timing)
    int GetJustCount() const { return justHitCount_; }

    // Reset
    void Reset();

private:
    int currentCombo_ = 0;
    int justHitCount_ = 0;
};

} // namespace Game
