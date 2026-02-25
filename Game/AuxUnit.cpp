#include "AuxUnit.h"
#include "Enemy.h"
#include "Projectile.h"

bool AuxUnit::Fire(const VECTOR& origin, Enemy* target, ProjectileManager& pm, bool consumeGauge, float& gauge, float cost)
{
    if (!Ready()) return false;
    if (consumeGauge && gauge < cost) return false;

    VECTOR aim = VGet(0,0,1);
    if (target) {
        VECTOR to = VSub(target->GetPosition(), origin);
        float len = VSize(to);
        if (len > 0.001f) aim = VScale(to, 1.0f / len);
    }

    pm.SpawnProjectile(type, origin, aim, dmg, speed);
    cd = cdMax;
    if (consumeGauge) gauge -= cost;
    return true;
}
