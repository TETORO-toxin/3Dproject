#pragma once
#include "DxLib.h"
#include <vector>
#include <cmath>

namespace Game {

// ==========================================
// Coin Structure - Represents a single coin projectile
// ==========================================
struct Coin {
    VECTOR pos;           // Current position (screen space: X=0~1280, Y=0~720)
    VECTOR velocity;      // Current velocity (vx, vy, 0)
    float mass = 1.0f;    // For physics calculations
    bool alive = true;    // Is this coin still active?
    float timeAlive = 0.0f; // Time since spawn (for sound desync calculation)
    bool atPeak = false;  // Has this coin reached its apex? (vy ≈ 0)
    int peakFramesSinceApex = 0; // Frames since we detected apex

    Coin() : pos(VGet(0, 0, 0)), velocity(VGet(0, 0, 0)) {}
};

// ==========================================
// CoinManager - Manages all active coins
// ==========================================
class CoinManager {
public:
    CoinManager();
    ~CoinManager() = default;

    // Spawn a single coin with given initial velocity
    void SpawnCoin(const VECTOR& startPos, const VECTOR& velocity);

    // Spawn multiple coins with slight angle variance (for multi-throw)
    void SpawnCoins(const VECTOR& startPos, float initialSpeed, float baseAngleDeg, int count);

    // Update all coins with physics
    void Update(float dt, float timescale = 1.0f);

    // Draw all coins as circles
    void Draw() const;

    // Get all active coins
    const std::vector<Coin>& GetCoins() const { return coins_; }
    std::vector<Coin>& GetCoinsRef() { return coins_; }

    // Clear all coins
    void Clear() { coins_.clear(); }

    // Check if any coins are active
    bool HasActiveCoin() const;

    // Get coin count
    int GetCoinCount() const { return (int)coins_.size(); }

private:
    std::vector<Coin> coins_;

    // Physics helper: Apply gravity and air resistance
    void ApplyPhysics(Coin& coin, float dt, float timescale);

    // Helper: Detect if coin is at apex (vy ≈ 0)
    void DetectApex(Coin& coin);
};

} // namespace Game
