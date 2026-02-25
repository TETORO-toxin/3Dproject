#pragma once
#include "DxLib.h"

class Enemy;
class ProjectileManager;

struct AuxUnit {
    int type = 0; // 0:MG,1:Laser,2:Missile...
    float cdMax = 0.5f; // seconds
    float cd = 0.0f;
    float dmg = 5.0f;
    float speed = 30.0f;

    AuxUnit() = default;
    AuxUnit(int t, float cdm, float dmg_, float spd) : type(t), cdMax(cdm), cd(0.0f), dmg(dmg_), speed(spd) {}

    void Update(float dt) {
        if (cd > 0.0f) cd -= dt;
        if (cd < 0.0f) cd = 0.0f;
    }

    bool Ready() const { return cd <= 0.0f; }

    // try to fire at target; returns true if fired
    bool Fire(const VECTOR& origin, Enemy* target, ProjectileManager& pm, bool consumeGauge, float& gauge, float cost);
};
