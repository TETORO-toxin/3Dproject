#include "SlowmotionManager.h"
#include "GameParams.h"

namespace Game {

SlowmotionManager::SlowmotionManager()
    : timescaleManager_(), justTimingSystem_(), comboSystem_()
{
}

void SlowmotionManager::ActivateSlow()
{
    timescaleManager_.SetTargetTimescale(Params::TIMESCALE_SLOW);
}

void SlowmotionManager::DeactivateSlow()
{
    timescaleManager_.SetTargetTimescale(Params::TIMESCALE_NORMAL);
}

void SlowmotionManager::RecordHit(const Coin& coin, bool isJust)
{
    comboSystem_.AddHit(coin, isJust);
}

void SlowmotionManager::Update(float dt)
{
    // Update timescale interpolation
    timescaleManager_.Update(dt);

    // Update just timing windows
    justTimingSystem_.Update();
}

void SlowmotionManager::Reset()
{
    timescaleManager_.SetTimescaleImmediate(Params::TIMESCALE_NORMAL);
    justTimingSystem_.Clear();
    comboSystem_.Reset();
}

} // namespace Game
