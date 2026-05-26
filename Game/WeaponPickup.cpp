// WeaponPickup.cpp
// äTóv:
//  - WeaponPickup ÇÃç≈è¨é¿ëï
//  - ï`âÊÇÕ DxLib ÇÃ DrawSphere3D Ç∆ DrawString ÇégÇ¡ÇΩâºé¿ëï

#include "WeaponPickup.h"
#include "WeaponTypes.h"
#include "../Sys/Assets.h"
#include "../Sys/DebugPrint.h"
#include <cstdio>
#include <cmath>

namespace Game {

WeaponPickup::WeaponPickup(WeaponType type, const VECTOR& pos, AssetsMgr* assets)
    : type_(type), pos_(pos), picked_(false), assets_(assets)
{
    // Initialize per-pickup transform from spec so each pickup can be
    // transformed independently of shared model handles. Do this regardless
    // of whether we successfully created a duplicated model instance.
    {
        const WeaponSpec& spec = GetWeaponSpec(type_);
        pickupScale_ = spec.pickupScale;
        pickupRotation_ = spec.pickupRotation;
    }

    // Try to create a dedicated instance model for this pickup so we can
    // freely transform it without affecting shared handles used for equipped models.
    if (assets_) {
        modelHandle_ = assets_->CreateWeaponModelInstance(type_, /*equip=*/false);
        // If creation failed, modelHandle_ stays -1 and Draw will fallback to marker.
        // Log the result so we can diagnose duplication failures.
        const WeaponSpec& spec = GetWeaponSpec(type_);
        const char* path = spec.pickupModelPath ? spec.pickupModelPath : "(null)";
        DebugPrint("WeaponPickup ctor: type=%d modelHandle=%d pickupPath=%s\n", static_cast<int>(type_), modelHandle_, path);
    }
}

WeaponPickup::~WeaponPickup()
{
    if (modelHandle_ != -1) {
        // The model was duplicated specifically for this pickup; delete it.
        MV1DeleteModel(modelHandle_);
        modelHandle_ = -1;
    }
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
    // Try to obtain a model handle from AssetsMgr. If unavailable or load failed,
    // fall back to the legacy cross marker drawing.
    int modelHandle = modelHandle_;

    // Always log the per-pickup model handle so we can verify whether this pickup
    // is using a dedicated duplicated model or falling back to the marker.
    DebugPrint("WeaponPickup::Draw: type=%d modelHandle=%d\n", static_cast<int>(type_), modelHandle);

    if (modelHandle == -1) {
        // Dedicated instance unavailable ? log and display which model path we attempted to duplicate.
        const WeaponSpec& spec = GetWeaponSpec(type_);
        const char* path = spec.pickupModelPath ? spec.pickupModelPath : "(null)";
        DebugPrint("WeaponPickup::Draw: dedicated model missing for type=%d pickupPath=%s\n", static_cast<int>(type_), path);
        char dbgBuf[512];
        sprintf_s(dbgBuf, "Pickup model missing: %s", path);
        DrawString(10, 90, dbgBuf, GetColor(255, 0, 0));
    }

    // Do NOT call assets_->GetWeaponModelHandle(...) for drawing. We only use
    // the per-pickup duplicated handle if available. Otherwise fall back to marker.

    if (modelHandle != -1) {
        // Apply pickup-specific transform from the per-pickup cached transform
        MV1SetPosition(modelHandle, pos_);
        MV1SetScale(modelHandle, VGet(pickupScale_, pickupScale_, pickupScale_));
        // pickupRotation_ stored as degrees in VECTOR; convert to radians for DXLib rotation API
        VECTOR rotDeg = pickupRotation_;
        float deg2rad = DX_PI_F / 180.0f;
        MV1SetRotationXYZ(modelHandle, VGet(rotDeg.x * deg2rad, rotDeg.y * deg2rad, rotDeg.z * deg2rad));
        MV1DrawModel(modelHandle);
    } else {
        unsigned int col = GetColor(200, 180, 120);
        // Draw a simple 3D cross as a pickup marker (avoid DrawSphere3D overload issues)
        const float s = 0.5f;
        DrawLine3D(VAdd(pos_, VGet(-s,0,0)), VAdd(pos_, VGet(s,0,0)), col);
        DrawLine3D(VAdd(pos_, VGet(0,-s,0)), VAdd(pos_, VGet(0,s,0)), col);
        DrawLine3D(VAdd(pos_, VGet(0,0,-s)), VAdd(pos_, VGet(0,0,s)), col);
    }

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
