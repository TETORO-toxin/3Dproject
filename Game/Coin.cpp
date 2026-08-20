#include "Coin.h"
#include "GameParams.h"
#include <cmath>
#include <algorithm>

namespace Game {

CoinManager::CoinManager() : coins_() {}

void CoinManager::SpawnCoin(const VECTOR& startPos, const VECTOR& velocity)
{
    Coin c;
    c.pos = startPos;
    c.velocity = velocity;
    c.alive = true;
    c.timeAlive = 0.0f;
    c.atPeak = false;
    c.peakFramesSinceApex = 0;
    coins_.push_back(c);
}

void CoinManager::SpawnCoins(const VECTOR& startPos, float initialSpeed, float baseAngleDeg, int count)
{
    // Spawn multiple coins with slight angle variance
    for (int i = 0; i < count; ++i) {
        // Add variance: each coin gets a slightly different angle
        float variance = (i - count / 2.0f) * Params::ANGLE_VARIANCE * 0.5f;
        float angleDeg = baseAngleDeg + variance;
        float angleRad = angleDeg * 3.14159265f / 180.0f;

        // Calculate velocity components
        float vx = initialSpeed * cosf(angleRad);
        float vy = initialSpeed * sinf(angleRad); // Positive Y is up in physics
        VECTOR vel = VGet(vx, vy, 0.0f);

        SpawnCoin(startPos, vel);
    }
}

void CoinManager::Update(float dt, float timescale)
{
    float scaledDt = dt * timescale;

    for (auto& coin : coins_) {
        if (!coin.alive) continue;

        coin.timeAlive += scaledDt;

        // Apply physics
        ApplyPhysics(coin, scaledDt, timescale);

        // Detect apex (for "KIN" sound timing)
        DetectApex(coin);

        // Remove coin if it falls below ground level
        if (coin.pos.y < Params::MAX_FALL_DISTANCE) {
            coin.alive = false;
        }
    }

    // Clean up dead coins
    coins_.erase(
        std::remove_if(coins_.begin(), coins_.end(), 
                       [](const Coin& c) { return !c.alive; }),
        coins_.end()
    );
}

void CoinManager::ApplyPhysics(Coin& coin, float dt, float timescale)
{
    // Apply gravity (downward acceleration)
    coin.velocity.y -= Params::GRAVITY * dt;

    // Apply air resistance
    coin.velocity.x *= Params::AIR_RESISTANCE;
    coin.velocity.y *= Params::AIR_RESISTANCE;

    // Update position
    coin.pos = VAdd(coin.pos, VScale(coin.velocity, dt));

    // Bounce off ground (simple ground collision)
    if (coin.pos.y <= Params::GROUND_LEVEL) {
        coin.pos.y = Params::GROUND_LEVEL;
        coin.velocity.y = -coin.velocity.y * 0.6f; // Energy loss on bounce
        coin.velocity.x *= 0.9f;
    }
}

void CoinManager::DetectApex(Coin& coin)
{
    // Check if velocity Y is close to zero (at apex)
    // We use a small threshold to handle floating-point imprecision
    const float APEX_THRESHOLD = 0.5f;

    if (fabsf(coin.velocity.y) < APEX_THRESHOLD && !coin.atPeak) {
        coin.atPeak = true;
        coin.peakFramesSinceApex = 0;
    }

    if (coin.atPeak) {
        coin.peakFramesSinceApex++;
    }
}

void CoinManager::Draw() const
{
    for (const auto& coin : coins_) {
        if (!coin.alive) continue;

        // Convert to screen coordinates and draw circle
        int x = (int)coin.pos.x;
        int y = (int)coin.pos.y;

        // Draw coin as a golden circle with outline
        DrawCircle(x, y, Params::COIN_RADIUS, GetColor(255, 215, 0), TRUE);  // Fill
        DrawCircle(x, y, Params::COIN_RADIUS, GetColor(200, 170, 0), FALSE, 2); // Outline

        // Debug: Show velocity vector
        if (false) { // Set to true for debug visualization
            int endX = x + (int)(coin.velocity.x * 2);
            int endY = y - (int)(coin.velocity.y * 2); // Negative Y because screen Y is inverted
            DrawLine(x, y, endX, endY, GetColor(255, 0, 0), 2);
        }
    }
}

bool CoinManager::HasActiveCoin() const
{
    for (const auto& coin : coins_) {
        if (coin.alive) return true;
    }
    return false;
}

} // namespace Game
