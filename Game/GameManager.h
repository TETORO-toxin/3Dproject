#pragma once
#include "DxLib.h"
#include "SlowmotionManager.h"
#include "Coin.h"
#include <vector>

namespace Game {

// ==========================================
// GameManager - Main game logic orchestrator
// ==========================================
class GameManager {
public:
    GameManager();
    ~GameManager() = default;

    // Initialize game
    bool Initialize();

    // Update game state
    void Update(float dt);

    // Draw game
    void Draw();

    // Check if game is running
    bool IsRunning() const { return isRunning_; }

    // Get slow-motion manager
    SlowmotionManager& GetSlowmotionManager() { return slowmotionManager_; }

private:
    bool isRunning_;
    SlowmotionManager slowmotionManager_;
    std::vector<Coin> coins_;
    
    // For testing purposes
    int frameCount_;
    bool slowToggle_;
};

} // namespace Game
