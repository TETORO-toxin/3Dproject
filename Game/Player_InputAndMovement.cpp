// Player_InputAndMovement.cpp
// サマリー:
//  - プレイヤーの入力処理と移動、ジャンプ/物理、モード切替/回避/攻撃入力の判定を担当します。
//  - カメラ参照を使ったカメラ基準移動とプレイヤーの向き設定、各種入力のデバウンスやクールダウン処理も含みます。

#include "Player.h"
#include "../Sys/Input.h"
#include "../Sys/DebugPrint.h"
#include "CameraRig.h"
#include <cmath>

// UpdateLogic: ゲームロジックのみを更新する（描画は行わない）
//  - 固定タイムステップで入力をポーリングし、モード切替、回避、補助発射判定、攻撃/ジャンプ入力の処理を行う。
//  - カメラ基準の移動ベクトルをワールドXZに変換し位置と向きを更新する。
//  - ジャンプや着地の垂直物理を統合し、適切なアニメーションへ遷移させる。
void Player::UpdateLogic()
{
    // 固定タイムステップ
    float dt = 1.0f / 60.0f;

    // コントローラまたはキーボード+マウスの統一入力
    InputState in = PollInput();

    // モード切替（例: A / space/Z）
    if (in.btnA) {
        unsigned int now = GetNowCount();
        if (now - lastModeSwitchTimeMs_ > 200) {
            lastModeSwitchTimeMs_ = now;
            mode_ = (mode_ == Mode::Melee) ? Mode::Ranged : Mode::Melee;
        }
    }

    // 回避（例: B / X）
    if (in.btnB) {
        unsigned int now = GetNowCount();
        if (now - lastDodgeTimeMs_ > dodgeCooldownMs_) {
            // ジャスト回避か判定
            if (now - lastIncomingTimeMs_ <= justWindowMs_) {
                justExecuted_ = true;
                // ジャスト回避ボーナス（短い前方テレポート）を適用
                x_ += 2.0f;
            }
            lastDodgeTimeMs_ = now;
        }
    }

    // 補助攻撃（左/右トリガーやマウスボタン）
    if (in.leftTrigger > 0.5f) {
        // 左補助発射 - 外部呼び出しを期待
    }
    if (in.rightTrigger > 0.5f) {
        // 右補助発射 - 外部呼び出しを期待
    }

    // 攻撃とジャンプ入力
    bool attackInput = false;
    bool jumpInput = false;
    // コントローラ割当
    if (in.btnX) attackInput = true;
    if (in.btnY) jumpInput = true;
    // マウス/キーボードの代替割当
    if (in.mouseLeft) attackInput = true;
    // キーボードジャンプ: モード切替と競合しないようCキーに割当
    if (CheckHitKey(KEY_INPUT_C)) jumpInput = true;

    // 左スティック/キーボードによる単純移動
    // 移動はカメラ基準: 前方入力でプレイヤーがカメラから遠ざかる（プレイヤーの背がカメラ向き）ように移動
    float moveX = in.moveX;
    float moveY = in.moveY;

    if (camera_ != nullptr) {
          VECTOR camF = camera_->GetForwardXZ(); // カメラからターゲット（プレイヤー）への正規化された前方ベクトル
          // プレイヤーの前方はカメラの前方とは逆向き（XZ平面）。前へ入力(moveY>0)でプレイヤーはカメラから離れる
          // XZ上の右ベクトルを計算
          VECTOR right = VGet(camF.z, 0.0f, -camF.x); // 90度回転
      
          // カメラ基準軸を使ってワールドXZの移動を合成。moveX=右, moveY=前
          float worldDX = right.x * moveX + (-camF.x) * moveY;
          float worldDZ = right.z * moveX + (-camF.z) * moveY;

          x_ += worldDX * 0.2f;
          z_ += worldDZ * 0.2f;

          // プレイヤーの向きを移動方向に合わせる
          {
              float faceDX = worldDX;
              float faceDZ = worldDZ;
              if (fabsf(faceDX) > 0.0001f || fabsf(faceDZ) > 0.0001f) {
                  float desiredYaw = atan2f(faceDX, faceDZ);
                  targetYaw_ = desiredYaw;
                  // step toward desired yaw using shortest angular distance
                  float a = currentYaw_;
                  float b = desiredYaw;
                  float diff = b - a;
                  // wrap to [-pi, pi]
                  while (diff > DX_PI_F) diff -= DX_PI_F * 2.0f;
                  while (diff < -DX_PI_F) diff += DX_PI_F * 2.0f;
                  float maxStep = yawTurnSpeed_ * dt;
                  if (fabsf(diff) <= maxStep) currentYaw_ = b;
                  else currentYaw_ += (diff > 0.0f ? 1.0f : -1.0f) * maxStep;

                  if (modelHandle_ != -1) MV1SetRotationXYZ(modelHandle_, VGet(0.0f, currentYaw_ + DX_PI_F, 0.0f));
                  if (baseModelHandle_ != -1 && baseModelHandle_ != modelHandle_) MV1SetRotationXYZ(baseModelHandle_, VGet(0.0f, currentYaw_ + DX_PI_F, 0.0f));
              }
          }
      } else {
          x_ += moveX * 0.2f;
          z_ += moveY * 0.2f;

          // プレイヤーの向きを移動方向に合わせる
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

                  if (modelHandle_ != -1) MV1SetRotationXYZ(modelHandle_, VGet(0.0f, currentYaw_ + DX_PI_F, 0.0f));
                  if (baseModelHandle_ != -1 && baseModelHandle_ != modelHandle_) MV1SetRotationXYZ(baseModelHandle_, VGet(0.0f, currentYaw_ + DX_PI_F, 0.0f));
              }
          }
      }
    // 地上にいる時、WASD/キーボード移動（またはコントローラ左スティック）でmoveアニメを再生
    bool moving = (fabsf(moveX) > 0.0001f || fabsf(moveY) > 0.0001f);
    if (onGround_) {
        // 攻撃やジャンプアニメを上書きしない
        if (currentAnim_ != "attack" && currentAnim_ != "jump") {
            if (moving) {
                if (currentAnim_ != "move") PlayAnimation("move", true, AnimLayer::Lower);
            } else {
                if (currentAnim_ != "idle") PlayAnimation("idle", true, AnimLayer::Lower);
            }
        }
    }
     
     // 攻撃入力の処理（移動より優先、ジャンプよりは劣後）
    unsigned int now = GetNowCount();

    // Edge-detect attack input so a single press triggers only one attack
    bool attackBtnComposite = attackInput;
    if (attackBtnComposite && !prevAttackBtnDown_) {
        // newly pressed
        if (now - lastAttackTimeMs_ > (unsigned int)attackCooldownMs_) {
            lastAttackTimeMs_ = now;
            // Play upper-body attack while keeping lower-body state (move/idle)
            PlayAnimation("attack", false, AnimLayer::Upper);
        }
    }
    prevAttackBtnDown_ = attackBtnComposite;

    // ジャンプ入力の処理
    if (jumpInput && onGround_) {
        unsigned int nowJ = GetNowCount();
        if (nowJ - lastJumpTimeMs_ > 200) {
            lastJumpTimeMs_ = nowJ;
            // ジャンプ開始
            velY_ = jumpVelocity_;
            onGround_ = false;
            // Jump is full-body, so play full animation (clears upper)
            PlayAnimation("jump", false, AnimLayer::Full);
        }
    }

    // 単純な垂直方向物理を統合
    if (!onGround_) {
        velY_ += gravity_ * dt;
        y_ += velY_ * dt;
        if (y_ <= 0.0f) {
            // 着地
            y_ = 0.0f;
            velY_ = 0.0f;
            onGround_ = true;
            // 着地時に移動またはアイドルアニメに復帰
            if (currentAnim_ != "attack") {
                // プレイヤーが移動中ならmove、そうでなければidle
                bool movingNow = (fabsf(in.moveX) > 0.0001f || fabsf(in.moveY) > 0.0001f);
                if (movingNow) PlayAnimation("move", true, AnimLayer::Lower); else PlayAnimation("idle", true, AnimLayer::Lower);
            }
        }
    }

    // アニメ更新（フレーム時間で進める）
    UpdateAnimation(dt);
}
