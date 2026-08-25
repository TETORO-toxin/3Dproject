#pragma once

// ==========================================
// THROALL'IN - Game Parameters Sheet
// Adjustable parameters for prototype tuning
// ==========================================

namespace Game {
namespace Params {

    // ========== Coin Physics ==========
    constexpr float COIN_INITIAL_SPEED = 30.0f;      // m/s (initial velocity)
    constexpr float GRAVITY = 9.8f;                  // m/s² (acceleration downward)
    constexpr float AIR_RESISTANCE = 0.99f;          // friction factor (0.0~1.0)
    constexpr float ANGLE_VARIANCE = 5.0f;           // degrees (±variance for multi-throw)

    // ========== Timescale ==========
    constexpr float TIMESCALE_NORMAL = 1.0f;         // Normal speed
    constexpr float TIMESCALE_SLOW = 0.2f;           // Slow-mo multiplier (adjustable later)
    constexpr float SLOW_TRANSITION_SPEED = 5.0f;    // frames to transition into slow

    // ========== Just Timing ==========
    constexpr int JUST_TOLERANCE_FRAMES = 3;         // ±3 frames 60fps = ~100ms
    constexpr float JUST_HIT_DAMAGE_MULT = 2.5f;     // Damage multiplier on just hit

    // ========== Reload System ==========
    constexpr int MAX_COINS = 5;                     // Maximum coin stock
    constexpr int MIN_COINS = 2;                     // Minimum coin stock (starting)
    constexpr float RELOAD_SPEED = 2.0f;             // bar travel speed (1.0 = full traverse per second)

    // ========== Damage System ==========
    constexpr float DAMAGE_PER_COIN = 10.0f;         // Base damage per coin hit
    constexpr float SELF_HIT_DAMAGE = 5.0f;          // Damage if shot misses and hits self

    // ========== Audio ==========
    // Sound file paths 
    constexpr const char* SOUND_KIN = "assets/sounds/kin.mp3";          // 「キン！」音
    constexpr const char* SOUND_SHOOT = "assets/sounds/shoot.mp3";      // 射撃音
    constexpr const char* SOUND_SELFHIT = "assets/sounds/selfhit.mp3";  // 自傷音

    // ========== Screen / Rendering ==========
    constexpr int SCREEN_WIDTH = 1280;
    constexpr int SCREEN_HEIGHT = 720;
    constexpr int PLAYER_RADIUS = 15;
    constexpr int COIN_RADIUS = 8;
    constexpr int TARGET_RADIUS = 20;

    // ========== Physics Limits ==========
    constexpr float GROUND_LEVEL = 0.0f;             // Y-coordinate below which coins disappear
    constexpr float MAX_FALL_DISTANCE = -100.0f;     // Hard limit for deletion

} // namespace Params
} // namespace Game
