// WeaponPickup.cpp
// äTóv:
//  - WeaponPickup ÇÃç≈è¨é¿ëï
//  - ï`âÊÇÕ DxLib ÇÃ DrawSphere3D Ç∆ DrawString ÇégÇ¡ÇΩâºé¿ëï

#include "WeaponPickup.h"
#include "WeaponTypes.h"
#include "../Sys/DebugPrint.h"
#include <cmath>

namespace Game {

WeaponPickup::WeaponPickup(WeaponType type, const VECTOR& pos)
    : type_(type), pos_(pos), picked_(false)
{
}

bool WeaponPickup::CanPickupBy(const VECTOR& playerPos, float range) const
{
    float dx = pos_.x - playerPos.x;
    float dz = pos_.z - playerPos.z;
    float dist2 = dx*dx + dz*dz;
    return (!picked_) && dist2 <= (range*range);
}

void WeaponPickup::Draw() const
{
    if (picked_) return;
    unsigned int col = GetColor(200, 180, 120);
    // Draw a simple 3D cross as a pickup marker (avoid DrawSphere3D overload issues)
    const float s = 0.5f;
    DrawLine3D(VAdd(pos_, VGet(-s,0,0)), VAdd(pos_, VGet(s,0,0)), col);
    DrawLine3D(VAdd(pos_, VGet(0,-s,0)), VAdd(pos_, VGet(0,s,0)), col);
    DrawLine3D(VAdd(pos_, VGet(0,0,-s)), VAdd(pos_, VGet(0,0,s)), col);

    // Draw name above the pickup
    const char* name = GetWeaponName(type_);
    VECTOR labelPos = VAdd(pos_, VGet(0.0f, 1.0f, 0.0f));
    VECTOR scr = ConvWorldPosToScreenPos(labelPos);
    // Only draw if in front of camera
    if (scr.z > 0.01f) {
        DrawString((int)scr.x - 16, (int)scr.y - 8, name, GetColor(255,255,255));
    }
}

} // namespace Game
