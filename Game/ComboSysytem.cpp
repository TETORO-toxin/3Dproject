#include "ComboSystem.h"
#include "GameParams.h"

namespace Game {

    ComboSystem::ComboSystem() : currentCombo_(0), justHitCount_(0) {}

    void ComboSystem::AddHit(const Coin& coin, bool isJust)
    {
        currentCombo_++;
        if (isJust) {
            justHitCount_++;
        }
    }

    float ComboSystem::GetComboMultiplier() const
    {
        if (currentCombo_ < 3) return 1.0f;
        if (currentCombo_ < 5) return 1.1f;
        if (currentCombo_ < 10) return 1.2f;
        if (currentCombo_ < 15) return 1.35f;
        if (currentCombo_ < 20) return 1.5f;
        return 1.75f;  // Max multiplier
    }

    void ComboSystem::BreakCombo()
    {
        currentCombo_ = 0;
        justHitCount_ = 0;
    }

    void ComboSystem::Reset()
    {
        BreakCombo();
    }

} // namespace Game