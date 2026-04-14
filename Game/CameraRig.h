#pragma once
#include "DxLib.h"

class CameraRig
{
public:
    CameraRig();
    void Update(); // default
    void Update(const VECTOR& playerPos, const VECTOR& targetPos, bool lockedOn);
    void SetTarget(float x, float y, float z);
    void SetGroundPlane(const VECTOR& point, const VECTOR& normal);

    // カメラの前方向を XZ 平面へ投影して正規化したベクトルを返す。
    VECTOR GetForwardXZ() const;

    // カメラ位置とフルフォワードベクトルの取得（ヘルパー）
    VECTOR GetCameraPosition() const;
    VECTOR GetForward() const;

private:
    VECTOR camPos_{0.0f, 12.0f, -24.0f};
    VECTOR camVel_{0.0f, 0.0f, 0.0f};
    VECTOR target_{0.0f, 6.0f, 0.0f};

    // ground represented as a point on the plane and a unit normal
    VECTOR groundPoint_{0.0f, 0.0f, 0.0f};
    VECTOR groundNormal_{0.0f, 1.0f, 0.0f};

    // spring params
    float springStiffness_ = 80.0f; // higher = stiffer
    float springDamping_ = 12.0f;

    // orbit state
    float yaw_ = 0.0f;   // radians
    float pitch_ = 0.25f; // radians (slightly looking down)
    int prevMouseX_ = -1;
    int prevMouseY_ = -1;

    // tuning
    float controllerYawSpeed_ = 1.5f; // rad/s at full stick (reduced for slower response)
    float controllerPitchSpeed_ = 1.2f; // reduced for slower response
    float mouseSensitivity_ = 0.003f; // rad per pixel (reduced for smoother mouse)
    float orbitRadius_ = 18.0f;

    // If true, camera is snapped directly above the player and should not be moved by inputs or springs.
    bool frozenAbove_ = false;
};
