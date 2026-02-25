#pragma once
#include "DxLib.h"

class Enemy
{
public:
    Enemy(const VECTOR& pos);
    ~Enemy();

    // Update only game logic (physics/state). Drawing is moved to Draw() so caller can control render order.
    void Update();
    void Draw();

    VECTOR GetPosition() const;

    // Combat-related
    float GetWeakGauge() const { return weakGauge; }
    bool  IsWeak() const { return isWeak; }
    bool  IsAirborne() const { return airborne; }

    void ApplyHitWEAK(float weakGain);
    void Launch();
    void FinishAssault();

private:
    int modelHandle_ = -1;
    VECTOR pos_;

    // WEAK / airborne
    float weakGauge = 0.0f; // 0..100
    bool  isWeak = false;
    bool  airborne = false;

    // simple vertical velocity for lift during launch
    float velY = 0.0f;
};
