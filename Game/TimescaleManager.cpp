#include "TimescaleManager.h"
#include "GameParams.h"

namespace Game {

TimescaleManager::TimescaleManager()
    : currentTimescale_(1.0f), targetTimescale_(1.0f), transitionSpeed_(Params::SLOW_TRANSITION_SPEED)
{
}

void TimescaleManager::SetTargetTimescale(float target)
{
    // Clamp to reasonable values
    targetTimescale_ = target;
    if (targetTimescale_ < 0.05f) targetTimescale_ = 0.05f;
    if (targetTimescale_ > 2.0f) targetTimescale_ = 2.0f;
}

void TimescaleManager::Update(float dt)
{
    // Interpolate current toward target
    if (currentTimescale_ < targetTimescale_) {
        currentTimescale_ += transitionSpeed_ * dt;
        if (currentTimescale_ > targetTimescale_) {
            currentTimescale_ = targetTimescale_;
        }
    } else if (currentTimescale_ > targetTimescale_) {
        currentTimescale_ -= transitionSpeed_ * dt;
        if (currentTimescale_ < targetTimescale_) {
            currentTimescale_ = targetTimescale_;
        }
    }
}

void TimescaleManager::SetTimescaleImmediate(float scale)
{
    SetTargetTimescale(scale);
    currentTimescale_ = scale;
}

} // namespace Game
