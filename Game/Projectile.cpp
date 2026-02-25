#include "Projectile.h"
#include <algorithm>

void Projectile::Update(float dt)
{
    pos = VAdd(pos, VScale(dir, speed * dt));
    // simple lifetime or world bounds check
    if (pos.x < -2000 || pos.x > 2000 || pos.z < -2000 || pos.z > 2000) alive = false;
}

void Projectile::Draw() const
{
    DrawBox((int)pos.x-2, (int)pos.y-2, (int)pos.x+2, (int)pos.y+2, GetColor(255,255,0), TRUE);
}

ProjectileManager::ProjectileManager()
{
}

ProjectileManager::~ProjectileManager()
{
}

void ProjectileManager::Update(float dt)
{
    for (auto &p : list_) p.Update(dt);
    list_.erase(std::remove_if(list_.begin(), list_.end(), [](const Projectile& p){ return !p.alive; }), list_.end());
}

void ProjectileManager::Draw() const
{
    for (const auto &p : list_) p.Draw();
}

void ProjectileManager::SpawnProjectile(int type, const VECTOR& origin, const VECTOR& dir, float dmg, float speed)
{
    Projectile pr;
    pr.type = type;
    pr.pos = origin;
    pr.dir = dir;
    pr.dmg = dmg;
    pr.speed = speed;
    pr.alive = true;
    list_.push_back(pr);
}
