// Player_InputAndMovement.cpp
// �T�}���[:
//  - �v���C���[�̓��͏����ƈړ��A�W�����v/�����A���[�h�ؑ�/����/�U�����͂̔������S�����܂��B
//  - �J�����Q�Ƃ��g�����J������ړ��ƃv���C���[�̌����ݒ��A�e�����͂̃f�o�E���X���N�[���_�E���������܂݂܂��B

#include "Player.h"
#include "../Sys/Input.h"
#include "../Sys/DebugPrint.h"
#include "CameraRig.h"
#include "../Sys/GlobalEffects.h"
#include <cmath>

// UpdateLogic: �Q�[�����W�b�N�݂̂��X�V�����i�`���͍s���Ȃ��j
//  - �Œ��^�C���X�e�b�v�œ��͂��|�[�����O���A���[�h�ؑցA�����A�⏕���˔����A�U��/�W�����v���͂̏������s���B
//  - �J������̈ړ��x�N�g�������[���hXZ�ɕϊ����ʒu�ƌ������X�V�����B
//  - �W�����v�⒅�n�̐��������𓝍����A�K�؂ȃA�j���[�V�����֑J�ڂ������B
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

        {
            float faceDX = worldDX;
            float faceDZ = worldDZ;
            if (fabsf(faceDX) > 0.0001f || fabsf(faceDZ) > 0.0001f) {
                float desiredYaw = atan2f(faceDX, faceDZ);
                targetYaw_ = desiredYaw;
                float a = currentYaw_;
                float b = desiredYaw;
                float diff = b - a;
                while (diff > DX_PI_F) diff -= DX_PI_F * 2.0f;
                while (diff < -DX_PI_F) diff += DX_PI_F * 2.0f;
                float maxStep = yawTurnSpeed_ * dt;
                if (fabsf(diff) <= maxStep) currentYaw_ = b;
                else currentYaw_ += (diff > 0.0f ? 1.0f : -1.0f) * maxStep;
            }
        }
    } else {
        x_ += moveX * 0.2f;
        z_ += moveY * 0.2f;

        {
            float faceDX = moveX;
            float faceDZ = moveY;
            if (fabsf(faceDX) > 0.0001f || fabsf(faceDZ) > 0.0001f) {
                float desiredYaw = atan2f(faceDX, faceDZ);
                targetYaw_ = desiredYaw;
                float a = currentYaw_;
                float b = desiredYaw;
                float diff = b - a;
                while (diff > DX_PI_F) diff -= DX_PI_F * 2.0f;
                while (diff < -DX_PI_F) diff += DX_PI_F * 2.0f;
                float maxStep = yawTurnSpeed_ * dt;
                if (fabsf(diff) <= maxStep) currentYaw_ = b;
                else currentYaw_ += (diff > 0.0f ? 1.0f : -1.0f) * maxStep;
            }
        }
    }

    bool moving = (fabsf(moveX) > 0.0001f || fabsf(moveY) > 0.0001f);
    if (onGround_) {
        if (currentAnim_ != "attack" && currentAnim_ != "attack_weapon" && currentAnim_ != "jump") {
            if (moving) {
                if (currentAnim_ != "move") PlayAnimation("move", true, AnimLayer::Lower);
            } else {
                const std::string desiredIdle = (equippedWeapon_ == Game::WeaponType::None) ? "idle" : "idle_weapon";
                if (currentAnim_ != desiredIdle) PlayAnimation(desiredIdle, true, AnimLayer::Lower);
            }
        }
    }

    unsigned int now = GetNowCount();

    bool attackBtnComposite = in.attackLightPressed || in.attackLightDown || in.attackHeavyPressed || in.attackHeavyDown;
    if (attackBtnComposite && !prevAttackBtnDown_) {
        if (now - lastAttackTimeMs_ > (unsigned int)attackCooldownMs_) {
            lastAttackTimeMs_ = now;
            const std::string attackName = (equippedWeapon_ == Game::WeaponType::None) ? "attack" : "attack_weapon";
            PlayAnimation(attackName, false, AnimLayer::Upper);
            EffectManager* gem = GetGlobalEffectManager();
            if (gem) {
                VECTOR forward = VGet(0.0f, 0.0f, 1.0f);
                if (camera_) forward = camera_->GetForwardXZ();
                float fl = sqrtf(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
                if (fl > 1e-6f) forward = VGet(forward.x/fl, forward.y/fl, forward.z/fl);

                const Game::WeaponSpec& wspec = Game::GetWeaponSpec(equippedWeapon_);
                const char* efFile = wspec.effectFile;
                float efScale = wspec.effectScale;
                VECTOR efOffset = wspec.effectOffset;

                VECTOR pos = GetPosition();
                pos = VAdd(pos, VGet(efOffset.x, efOffset.y, 0.0f));
                pos = VAdd(pos, VScale(forward, efOffset.z));

                gem->PlayEffectAt(pos, efFile, efScale);
            }
        }
    }
    prevAttackBtnDown_ = attackBtnComposite;

    if ((in.jumpPressed || in.jumpDown) && onGround_) {
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

void Player::DrawCoinThrowEffect() const
{
    if (coinAlive_)
    {
        DrawSphere3D(
            coinPos_,
            0.25f,
            16,
            GetColor(255, 210, 80),
            GetColor(255, 180, 20),
            GetColor(0, 0, 0),
            TRUE);
    }
}

void Player::DrawReflectTrajectory() const
{
    if (!reflectShotDrawn_) return;

    VECTOR muzzle = VAdd(GetPosition(), VGet(0.0f, 1.4f, 0.0f));
    muzzle = VAdd(muzzle, VScale(GetPlayerForward(this, camera_), 1.5f));

    DrawLine3D(muzzle, coinPos_, GetColor(255, 220, 90));
    DrawLine3D(coinPos_, reflectTarget_, reflectHit_ ? GetColor(0, 255, 120) : GetColor(255, 90, 90));

    DrawSphere3D(
        reflectTarget_,
        0.18f,
        10,
        reflectHit_ ? GetColor(0, 255, 150) : GetColor(255, 120, 120),
        reflectHit_ ? GetColor(40, 180, 90) : GetColor(180, 60, 60),
        GetColor(0, 0, 0),
        TRUE);
}
