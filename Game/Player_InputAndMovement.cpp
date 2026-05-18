// Player_InputAndMovement.cpp
// サマリー:
//  - プレイヤーの入力処理と移動、ジャンプ/物理、モード切替/回避/攻撃入力の判定を担当します。
//  - カメラ参照を使ったカメラ基準移動とプレイヤーの向き設定、各種入力のデバウンスやクールダウン処理も含みます。

#include "Player.h"
#include "../Sys/Input.h"
#include "../Sys/DebugPrint.h"
#include "CameraRig.h"
#include "../Sys/GlobalEffects.h"
#include <cmath>

// UpdateLogic: ゲームロジックのみを更新する（描画は行わない）
//  - 固定タイムステップで入力をポーリングし、モード切替、回避、補助発射判定、攻撃/ジャンプ入力の処理を行う。
//  - カメラ基準の移動ベクトルをワールドXZに変換し位置と向きを更新する。
//  - ジャンプや着地の垂直物理を統合し、適切なアニメーションへ遷移させる。
void Player::UpdateLogic(float dt, const InputState& in)
{
    // dt は SceneMgr から渡された値（秒）

    // 現状は近接武器のみのためモード切替は無効化されています。
    // 将来の射撃武器実装に向けた TODO として残しています。

    // 回避（意味ベースのフラグを使用）
    if (in.dodgePressed || (in.dodgeDown && !in.dodgePressed && false)) {
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

    // 補助攻撃（左/右トリガーやマウスボタン） - 意味ベースの閾値を使用
    if (in.leftTrigger > 0.5f || in.leftTriggerHoldTime > 0.0f) {
        // 左補助発射 - 外部呼び出しを期待
    }
    if (in.rightTrigger > 0.5f || in.rightTriggerHoldTime > 0.0f) {
        // 右補助発射 - 外部呼び出しを期待
    }

    // 攻撃とジャンプ入力: InputState の意味的アクションフラグを使用
    bool attackInput = in.attackLightPressed || in.attackLightDown || in.attackHeavyPressed || in.attackHeavyDown;
    bool jumpInput = in.jumpPressed || in.jumpDown;

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
                  // 最短の角距離で目標ヨーへ刻み移動する
                  float a = currentYaw_;
                  float b = desiredYaw;
                  float diff = b - a;
                  // 巻き込みを行い角差を [-pi, pi] に正規化
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
                  // 巻き込みを行い角差を [-pi, pi] に正規化
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

    // 攻撃入力をエッジ検出して、1 回の押下で 1 回だけ攻撃が発動するようにする
    bool attackBtnComposite = attackInput;
    if (attackBtnComposite && !prevAttackBtnDown_) {
        // 押下開始（エッジ）
        if (now - lastAttackTimeMs_ > (unsigned int)attackCooldownMs_) {
            lastAttackTimeMs_ = now;
            // 下半身状態（move/idle）を維持しつつ上半身攻撃アニメを再生
            PlayAnimation("attack", false, AnimLayer::Upper);
            // spawn an attack effect slightly in front of the player
            // 変更点: 装備武器 (equippedWeapon_) に合わせてエフェクトのファイル/スケール/オフセットを選択する
            // - WeaponTypes のヘルパー GetWeaponEffectFile/Scale/Offset を使う
            // - 装備なし (None) でも攻撃は行えるが、エフェクトは弱めにする
            EffectManager* gem = GetGlobalEffectManager();
            if (gem) {
                VECTOR forward = VGet(0.0f, 0.0f, 1.0f);
                if (camera_) forward = camera_->GetForwardXZ();
                float fl = sqrtf(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
                if (fl > 1e-6f) forward = VGet(forward.x/fl, forward.y/fl, forward.z/fl);

                // WeaponSpec から差分を取得
                const Game::WeaponSpec& wspec = Game::GetWeaponSpec(equippedWeapon_);
                const char* efFile = wspec.effectFile;
                float efScale = wspec.effectScale;
                VECTOR efOffset = wspec.effectOffset;

                // オフセットの z 成分は前方に乗算して扱う (forward を正規化しているため、z 成分だけを使うのではなく
                // forward ベクトルに対して efOffset.z を掛ける)
                VECTOR pos = GetPosition();
                // 高さを efOffset.y、左右は efOffset.x、前方方向に efOffset.z
                pos = VAdd(pos, VGet(efOffset.x, efOffset.y, 0.0f));
                pos = VAdd(pos, VScale(forward, efOffset.z));

                // PlayEffectAt はファイルパスとスケールを受け取れるためそれらを渡す。
                gem->PlayEffectAt(pos, efFile, efScale);
            }
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
            // ジャンプは全身アニメなのでフルレイヤー再生（上半身アニメをクリア）
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
