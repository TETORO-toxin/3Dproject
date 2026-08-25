#include "JustTimingSystem.h"
#include "GameParams.h"
#include <algorithm>

namespace Game {

JustTimingSystem::JustTimingSystem() : justWindows_() {}

void JustTimingSystem::StartJustWindow(int coinIndex)
{
    // Add a new just window for this coin
    JustWindow window;
    window.coinIndex = coinIndex;
    window.framesRemaining = Params::JUST_TOLERANCE_FRAMES;
    justWindows_.push_back(window);
}

bool JustTimingSystem::IsJustFrame(int coinIndex) const
{
    for (const auto& window : justWindows_) {
        if (window.coinIndex == coinIndex && window.framesRemaining > 0) {
            return true;
        }
    }
    return false;
}

void JustTimingSystem::Update()
{
    // Decrement frame counters
    for (auto& window : justWindows_) {
        window.framesRemaining--;
    }

    // Remove expired windows
    justWindows_.erase(
        std::remove_if(justWindows_.begin(), justWindows_.end(),
                       [](const JustWindow& w) { return w.framesRemaining <= 0; }),
        justWindows_.end()
    );
}

void JustTimingSystem::Clear()
{
    justWindows_.clear();
}

} // namespace Game
