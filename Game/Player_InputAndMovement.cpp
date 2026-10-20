#include "Player.h"
#include "../Sys/Input.h"
#include "../Sys/DebugPrint.h"
#include "CameraRig.h"
#include "../Sys/GlobalEffects.h"
#include "Enemy.h"
#include <cmath>
#include <algorithm>

static inline VECTOR Normalize3D(const VECTOR& v)
{
    float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 0.0001f) return VGet(0.0f, 0.0f, 1.0f);
    return VGet(v.x / len, v.y / len, v.z / len);
}

static inline VECTOR GetPlayerForward(const Player* player, CameraRig* camera)
{
    if (camera) {
        VECTOR f = camera->GetForwardXZ();
        float len = sqrtf(f.x * f.x + f.z * f.z);
        if (len > 0.0001f) return Normalize3D(VGet(f.x, 0.0f, f.z));
    }
    return VGet(0.0f, 0.0f, 1.0f);
}

void Player::SetCoinFocusTarget(const VECTOR& coinPos)
{
    coinFocusTarget_ = coinPos;
}

void Player::UpdateCoinThrow(float dt, const InputState& in, const std::vector<Enemy*>& enemies)
{
    bool attackKeyDown = in.attackLightDown || in.attackHeavyDown || in.mouseLeft || in.mouseRight;
    bool attackKeyPressed = in.attackLightPressed || in.attackHeavyPressed || in.mouseLeft || in.mouseRight;

    if (state_ == PlayerState::STATE_NORMAL)
    {
        if (attackKeyPressed)
        {
            state_ = PlayerState::STATE_THROW_CHARGING;
            chargeTime_ = 0.0f;
            coinAlive_ = true;
            attackKeyHeld_ = true;
            reflectHitEnemy_ = nullptr;

            VECTOR forward = GetPlayerForward(this, camera_);
            VECTOR muzzlePos = VAdd(GetPosition(), VGet(0.0f, 1.5f, 0.0f));
            muzzlePos = VAdd(muzzlePos, VScale(forward, 1.5f));

            coinPos_ = muzzlePos;
            coinVel_ = VGet(forward.x * 6.5f, 5.5f, forward.z * 6.5f);
            currentTimeScale_ = slowMotionScale_;
            reflectShotDrawn_ = false;
            reflectHit_ = false;
            reflectTarget_ = coinPos_;
            reflectDrawTimer_ = 0.0f;
        }
    }

    if (state_ == PlayerState::STATE_THROW_CHARGING)
    {
        attackKeyHeld_ = attackKeyDown;
        float scaledDt = dt * slowMotionScale_;
        chargeTime_ += scaledDt;

        coinVel_.y -= 18.0f * scaledDt;
        coinPos_ = VAdd(coinPos_, VScale(coinVel_, scaledDt));

        if (coinPos_.y <= 0.0f)
        {
            coinPos_.y = 0.0f;
            coinAlive_ = false;
            state_ = PlayerState::STATE_NORMAL;
            currentTimeScale_ = 1.0f;
            reflectShotDrawn_ = false;
            return;
        }

        coinFocusTarget_ = coinPos_;

        if (!attackKeyDown)
        {
            Enemy* bestEnemy = nullptr;
            float bestDistSq = 999999.0f;
            for (Enemy* e : enemies)
            {
                if (!e) continue;
                VECTOR diff = VSub(e->GetPosition(), coinPos_);
                float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                if (distSq < bestDistSq)
                {
                    bestDistSq = distSq;
                    bestEnemy = e;
                }
            }

            reflectHitEnemy_ = nullptr;
            bool hitEnemy = (bestEnemy != nullptr && bestDistSq < 25.0f);
            if (hitEnemy)
            {
                reflectHitEnemy_ = bestEnemy;
                reflectHit_ = true;
                reflectTarget_ = bestEnemy->GetPosition();
                bestEnemy->ApplyHitWEAK(100.0f);
                bestEnemy->Launch();
                DebugPrint("Enemy weak-hit confirmed: enemy at (%.2f, %.2f, %.2f)\n",
                    reflectTarget_.x, reflectTarget_.y, reflectTarget_.z);
            }
            else
            {
                reflectHit_ = false;
                reflectTarget_ = VAdd(coinPos_, VGet(
                    (GetRand(200) - 100) * 0.05f,
                    1.0f + (GetRand(100) * 0.02f),
                    (GetRand(200) - 100) * 0.05f));
            }

            reflectShotDrawn_ = true;
            reflectDrawTimer_ = 0.8f;
            coinAlive_ = false;
            state_ = PlayerState::STATE_NORMAL;
            currentTimeScale_ = 1.0f;
            return;
        }
    }
}

void Player::DrawCoinThrowEffect() const
{
    if (coinAlive_)
    {
        DrawSphere3D(coinPos_, 0.25f, 16, 16,
            GetColor(255, 210, 80), GetColor(255, 180, 20), TRUE);
    }
}

void Player::DrawReflectTrajectory() const
{
    if (!reflectShotDrawn_) return;

    VECTOR muzzle = VAdd(GetPosition(), VGet(0.0f, 1.4f, 0.0f));
    muzzle = VAdd(muzzle, VScale(GetPlayerForward(this, camera_), 1.5f));

    DrawLine3D(muzzle, coinPos_, GetColor(255, 220, 90));
    DrawLine3D(coinPos_, reflectTarget_, reflectHit_ ? GetColor(0, 255, 120) : GetColor(255, 90, 90));

    DrawSphere3D(reflectTarget_, 0.18f, 10, 10,
        reflectHit_ ? GetColor(0, 255, 150) : GetColor(255, 120, 120),
        reflectHit_ ? GetColor(40, 180, 90) : GetColor(180, 60, 60),
        TRUE);
}

void Player::UpdateLogic(float dt, const InputState& in)
{
    float moveX = in.moveX;
    float moveY = in.moveY;

    if (camera_ != nullptr) {
        VECTOR camF = camera_->GetForwardXZ();
        VECTOR right = VGet(camF.z, 0.0f, -camF.x);
        float worldDX = right.x * moveX + (-camF.x) * moveY;
        float worldDZ = right.z * moveX + (-camF.z) * moveY;

        x_ += worldDX * 0.2f;
        z_ += worldDZ * 0.2f;
    } else {
        x_ += moveX * 0.2f;
        z_ += moveY * 0.2f;
    }

    if (onGround_) {
        bool moving = (fabsf(moveX) > 0.0001f || fabsf(moveY) > 0.0001f);
        if (currentAnim_ != "attack" && currentAnim_ != "attack_weapon" && currentAnim_ != "jump") {
            if (moving) {
                if (currentAnim_ != "move") PlayAnimation("move", true, AnimLayer::Lower);
            } else {
                const std::string desiredIdle = (equippedWeapon_ == Game::WeaponType::None) ? "idle" : "idle_weapon";
                if (currentAnim_ != desiredIdle) PlayAnimation(desiredIdle, true, AnimLayer::Lower);
            }
        }
    }

    bool jumpInput = in.jumpPressed || in.jumpDown;
    if (jumpInput && onGround_) {
        unsigned int nowJ = GetNowCount();
        if (nowJ - lastJumpTimeMs_ > 200) {
            lastJumpTimeMs_ = nowJ;
            velY_ = jumpVelocity_;
            onGround_ = false;
            PlayAnimation("jump", false, AnimLayer::Full);
        }
    }

    if (!onGround_) {
        velY_ += gravity_ * dt;
        y_ += velY_ * dt;
        if (y_ <= 0.0f) {
            y_ = 0.0f;
            velY_ = 0.0f;
            onGround_ = true;
            if (currentAnim_ != "attack" && currentAnim_ != "attack_weapon") {
                bool movingNow = (fabsf(in.moveX) > 0.0001f || fabsf(in.moveY) > 0.0001f);
                if (movingNow) PlayAnimation("move", true, AnimLayer::Lower);
                else {
                    const std::string desiredIdle = (equippedWeapon_ == Game::WeaponType::None) ? "idle" : "idle_weapon";
                    PlayAnimation(desiredIdle, true, AnimLayer::Lower);
                }
            }
        }
    }

    UpdateAnimation(dt);
}
