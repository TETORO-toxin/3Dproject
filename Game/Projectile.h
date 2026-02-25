#pragma once
#include "DxLib.h"
#include <vector>

struct Projectile {
    int type = 0;
    VECTOR pos;
    VECTOR dir; // normalized
    float speed = 30.0f;
    float dmg = 1.0f;
    bool alive = true;

    void Update(float dt);
    void Draw() const;
};

class ProjectileManager {
public:
    ProjectileManager();
    ~ProjectileManager();

    void Update(float dt);
    void Draw() const;

    void SpawnProjectile(int type, const VECTOR& origin, const VECTOR& dir, float dmg, float speed);
    const std::vector<Projectile>& GetProjectiles() const { return list_; } // Expose list_

private:
    std::vector<Projectile> list_;
};
