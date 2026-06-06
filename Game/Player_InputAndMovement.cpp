// Player_InputAndMovement.cpp
// �T�}���[:
//  - �v���C���[�̓��͏����ƈړ��A�W�����v/�����A���[�h�ؑ�/���/�U�����͂̔����S�����܂��B
//  - �J�����Q�Ƃ��g�����J������ړ��ƃv���C���[�̌����ݒ�A�e����͂̃f�o�E���X��N�[���_�E���������܂݂܂��B

#include "Player.h"
#include "../Sys/Input.h"
#include "../Sys/DebugPrint.h"
#include "CameraRig.h"
#include "../Sys/GlobalEffects.h"
#include <cmath>

// UpdateLogic: �Q�[�����W�b�N�݂̂��X�V����i�`��͍s��Ȃ��j
//  - �Œ�^�C���X�e�b�v�œ��͂��|�[�����O���A���[�h�ؑցA����A�⏕���˔���A�U��/�W�����v���͂̏������s���B
//  - �J������̈ړ��x�N�g�������[���hXZ�ɕϊ����ʒu�ƌ������X�V����B
//  - �W�����v�⒅�n�̐��������𓝍����A�K�؂ȃA�j���[�V�����֑J�ڂ�����B
void Player::UpdateLogic(float dt, const InputState& in)
{
    // dt �� SceneMgr ����n���ꂽ�l�i�b�j

    // ����͋ߐڕ���݂̂̂��߃��[�h�ؑւ͖���������Ă��܂��B
    // �����̎ˌ���������Ɍ����� TODO �Ƃ��Ďc���Ă��܂��B

    // ����i�Ӗ��x�[�X�̃t���O���g�p�j
    if (in.dodgePressed || (in.dodgeDown && !in.dodgePressed && false)) {
        unsigned int now = GetNowCount();
        if (now - lastDodgeTimeMs_ > dodgeCooldownMs_) {
            // �W���X�g���������
            if (now - lastIncomingTimeMs_ <= justWindowMs_) {
                justExecuted_ = true;
                // �W���X�g����{�[�i�X�i�Z���O���e���|�[�g�j��K�p
                x_ += 2.0f;
            }
            lastDodgeTimeMs_ = now;
        }
    }

    // �⏕�U���i��/�E�g���K�[��}�E�X�{�^���j - �Ӗ��x�[�X��臒l���g�p
    if (in.leftTrigger > 0.5f || in.leftTriggerHoldTime > 0.0f) {
        // ���⏕���� - �O���Ăяo��������
    }
    if (in.rightTrigger > 0.5f || in.rightTriggerHoldTime > 0.0f) {
        // �E�⏕���� - �O���Ăяo��������
    }

    // �U���ƃW�����v����: InputState �̈Ӗ��I�A�N�V�����t���O���g�p
    bool attackInput = in.attackLightPressed || in.attackLightDown || in.attackHeavyPressed || in.attackHeavyDown;
    bool jumpInput = in.jumpPressed || in.jumpDown;

    // ���X�e�B�b�N/�L�[�{�[�h�ɂ��P���ړ�
    // �ړ��̓J�����: �O�����͂Ńv���C���[���J�������牓������i�v���C���[�̔w���J���������j�悤�Ɉړ�
    float moveX = in.moveX;
    float moveY = in.moveY;

    if (camera_ != nullptr) {
          VECTOR camF = camera_->GetForwardXZ(); // �J��������^�[�Q�b�g�i�v���C���[�j�ւ̐��K�����ꂽ�O���x�N�g��
          // �v���C���[�̑O���̓J�����̑O���Ƃ͋t�����iXZ���ʁj�B�O�֓���(moveY>0)�Ńv���C���[�̓J�������痣���
          // XZ��̉E�x�N�g�����v�Z
          VECTOR right = VGet(camF.z, 0.0f, -camF.x); // 90�x��]
      
          // �J����������g���ă��[���hXZ�̈ړ��������BmoveX=�E, moveY=�O
          float worldDX = right.x * moveX + (-camF.x) * moveY;
          float worldDZ = right.z * moveX + (-camF.z) * moveY;

          x_ += worldDX * 0.2f;
          z_ += worldDZ * 0.2f;

          // �v���C���[�̌������ړ������ɍ��킹��
          {
              float faceDX = worldDX;
              float faceDZ = worldDZ;
              if (fabsf(faceDX) > 0.0001f || fabsf(faceDZ) > 0.0001f) {
                  float desiredYaw = atan2f(faceDX, faceDZ);
                  targetYaw_ = desiredYaw;
                  // �ŒZ�̊p�����ŖڕW���[�֍��݈ړ�����
                  float a = currentYaw_;
                  float b = desiredYaw;
                  float diff = b - a;
                  // �������݂��s���p���� [-pi, pi] �ɐ��K��
                  while (diff > DX_PI_F) diff -= DX_PI_F * 2.0f;
                  while (diff < -DX_PI_F) diff += DX_PI_F * 2.0f;
                  float maxStep = yawTurnSpeed_ * dt;
                  if (fabsf(diff) <= maxStep) currentYaw_ = b;
                  else currentYaw_ += (diff > 0.0f ? 1.0f : -1.0f) * maxStep;

                  // Rotation applied during Draw() to keep transform updates centralized.
                  // Avoid setting MV1 model rotation here to prevent inconsistent orientation.
              }
          }
      } else {
          x_ += moveX * 0.2f;
          z_ += moveY * 0.2f;

          // �v���C���[�̌������ړ������ɍ��킹��
          {
              float faceDX = moveX;
              float faceDZ = moveY;
              if (fabsf(faceDX) > 0.0001f || fabsf(faceDZ) > 0.0001f) {
                  float desiredYaw = atan2f(faceDX, faceDZ);
                  targetYaw_ = desiredYaw;
                  float a = currentYaw_;
                  float b = desiredYaw;
                  float diff = b - a;
                  // �������݂��s���p���� [-pi, pi] �ɐ��K��
                  while (diff > DX_PI_F) diff -= DX_PI_F * 2.0f;
                  while (diff < -DX_PI_F) diff += DX_PI_F * 2.0f;
                  float maxStep = yawTurnSpeed_ * dt;
                  if (fabsf(diff) <= maxStep) currentYaw_ = b;
                  else currentYaw_ += (diff > 0.0f ? 1.0f : -1.0f) * maxStep;

                  // Rotation applied during Draw() to keep transform updates centralized.
                  // Avoid setting MV1 model rotation here to prevent inconsistent orientation.
              }
          }
      }
    // �n��ɂ��鎞�AWASD/�L�[�{�[�h�ړ��i�܂��̓R���g���[�����X�e�B�b�N�j��move�A�j�����Đ�
    bool moving = (fabsf(moveX) > 0.0001f || fabsf(moveY) > 0.0001f);
    if (onGround_) {
        // �U����W�����v�A�j�����㏑�����Ȃ�
        if (currentAnim_ != "attack" && currentAnim_ != "attack_weapon" && currentAnim_ != "jump") {
            if (moving) {
                if (currentAnim_ != "move") PlayAnimation("move", true, AnimLayer::Lower);
            } else {
                const bool hasWeapon = (equippedWeapon_ != Game::WeaponType::None);
                const std::string idleAnim = hasWeapon ? "idle_weapon" : "idle";
                if (currentAnim_ != idleAnim) PlayAnimation(idleAnim, true, AnimLayer::Lower);
            }
        }
    }
     
     // �U�����͂̏����i�ړ����D��A�W�����v���͗��j
    unsigned int now = GetNowCount();

    // �U�����͂��G�b�W���o���āA1 ��̉����� 1 �񂾂��U������������悤�ɂ���
    bool attackBtnComposite = attackInput;
    if (attackBtnComposite && !prevAttackBtnDown_) {
        // �����J�n�i�G�b�W�j
        if (now - lastAttackTimeMs_ > (unsigned int)attackCooldownMs_) {
            lastAttackTimeMs_ = now;
            // �����g��ԁimove/idle�j���ێ����㔼�g�U���A�j�����Đ�
            const bool hasWeapon = (equippedWeapon_ != Game::WeaponType::None);
            PlayAnimation(hasWeapon ? "attack_weapon" : "attack", false, AnimLayer::Upper);
            // spawn an attack effect slightly in front of the player
            // �ύX�_: �������� (equippedWeapon_) �ɍ��킹�ăG�t�F�N�g�̃t�@�C��/�X�P�[��/�I�t�Z�b�g��I������
            // - WeaponTypes �̃w���p�[ GetWeaponEffectFile/Scale/Offset ���g��
            // - �����Ȃ� (None) �ł��U���͍s���邪�A�G�t�F�N�g�͎�߂ɂ���
            EffectManager* gem = GetGlobalEffectManager();
            if (gem) {
                VECTOR forward = VGet(0.0f, 0.0f, 1.0f);
                if (camera_) forward = camera_->GetForwardXZ();
                float fl = sqrtf(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
                if (fl > 1e-6f) forward = VGet(forward.x/fl, forward.y/fl, forward.z/fl);

                // WeaponSpec ���獷�����擾
                const Game::WeaponSpec& wspec = Game::GetWeaponSpec(equippedWeapon_);
                const char* efFile = wspec.effectFile;
                float efScale = wspec.effectScale;
                VECTOR efOffset = wspec.effectOffset;

                // �I�t�Z�b�g�� z �����͑O���ɏ�Z���Ĉ��� (forward �𐳋K�����Ă��邽�߁Az �����������g���̂ł͂Ȃ�
                // forward �x�N�g���ɑ΂��� efOffset.z ���|����)
                VECTOR pos = GetPosition();
                // ������ efOffset.y�A���E�� efOffset.x�A�O�������� efOffset.z
                pos = VAdd(pos, VGet(efOffset.x, efOffset.y, 0.0f));
                pos = VAdd(pos, VScale(forward, efOffset.z));

                // PlayEffectAt �̓t�@�C���p�X�ƃX�P�[�����󂯎��邽�߂�����n���B
                gem->PlayEffectAt(pos, efFile, efScale);
            }
        }
    }
    prevAttackBtnDown_ = attackBtnComposite;

    // �W�����v���͂̏���
    if (jumpInput && onGround_) {
        unsigned int nowJ = GetNowCount();
        if (nowJ - lastJumpTimeMs_ > 200) {
            lastJumpTimeMs_ = nowJ;
            // �W�����v�J�n
            velY_ = jumpVelocity_;
            onGround_ = false;
            // �W�����v�͑S�g�A�j���Ȃ̂Ńt�����C���[�Đ��i�㔼�g�A�j�����N���A�j
            PlayAnimation("jump", false, AnimLayer::Full);
        }
    }

    // �P���Ȑ������������𓝍�
    if (!onGround_) {
        velY_ += gravity_ * dt;
        y_ += velY_ * dt;
        if (y_ <= 0.0f) {
            // ���n
            y_ = 0.0f;
            velY_ = 0.0f;
            onGround_ = true;
            // ���n���Ɉړ��܂��̓A�C�h���A�j���ɕ��A
            if (currentAnim_ != "attack" && currentAnim_ != "attack_weapon") {
                // �v���C���[���ړ����Ȃ�move�A�����łȂ����idle
                bool movingNow = (fabsf(in.moveX) > 0.0001f || fabsf(in.moveY) > 0.0001f);
                if (movingNow) {
                    PlayAnimation("move", true, AnimLayer::Lower);
                } else {
                    const bool hasWeapon = (equippedWeapon_ != Game::WeaponType::None);
                    PlayAnimation(hasWeapon ? "idle_weapon" : "idle", true, AnimLayer::Lower);
                }
            }
        }
    }

    // �A�j���X�V�i�t���[�����ԂŐi�߂�j
    UpdateAnimation(dt);
}
