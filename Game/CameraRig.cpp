#include "CameraRig.h"
#include "../Sys/Input.h"
#include <cmath>

static float Dot(const VECTOR& a, const VECTOR& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static VECTOR Normalize(const VECTOR& v) {
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len < 0.000001f) return VGet(0,0,1);
    return VGet(v.x/len, v.y/len, v.z/len);
}

CameraRig::CameraRig()
{
}

void CameraRig::SetTarget(float x, float y, float z)
{
    target_.x = x; target_.y = y; target_.z = z;
}

void CameraRig::SetGroundPlane(const VECTOR& point, const VECTOR& normal)
{
    groundPoint_ = point;
    groundNormal_ = Normalize(normal);
}

void CameraRig::Update()
{
    SetCameraNearFar(0.1f, 1200.0f);
    SetCameraPositionAndTarget_UpVecY(camPos_, target_);
}

void CameraRig::Update(const VECTOR& playerPos, const VECTOR& targetPos, bool lockedOn)
{
    // この更新関数は毎フレーム呼び出され、カメラの位置・向きを計算します。
    // playerPos: プレイヤーのワールド座標
    // targetPos: ロックオン対象などのターゲット座標
    // lockedOn: ロックオン中かどうかのフラグ

    SetCameraNearFar(0.1f, 1200.0f);

    VECTOR desired;

    // コントローラの入力統合とスプリング計算のタイムステップ
    float dt = 1.0f / 60.0f;

    // Poll inputs
    PadState pad = PollPad(0);
    InputState in = PollInput(0);

    // コントローラが接続されているか判定する（PollInput と同じヒューリスティック）
    bool controllerPresent = (pad.pad != 0) || (pad.xi.ThumbLX != 0) || (pad.xi.ThumbLY != 0) || (pad.xi.ThumbRX != 0) || (pad.xi.ThumbRY != 0) || (pad.xi.LeftTrigger != 0) || (pad.xi.RightTrigger != 0);

    if (lockedOn) {
        // ロックオン時: 以前の挙動を維持（プレイヤーの背後からターゲットを見る）
        // ターゲットの方向に基づいてカメラをプレイヤーの背後に配置し、少し上方を見る
        VECTOR forward = VSub(targetPos, playerPos);
        float len = VSize(forward);
        if (len > 0.0001f) forward = VScale(forward, 1.0f / len);
        else forward = VGet(0.0f, 0.0f, 1.0f);

        desired = VAdd(playerPos, VScale(forward, -12.0f));
        desired.y += 6.0f;
        target_ = targetPos;
        frozenAbove_ = false; // allow movement when returning to normal
    } else {
        // 通常時: オービット処理やマウス/スティック入力による回転を行う
        // frozenAbove_ が true のときは、プレイヤーの真上に固定されているので回転処理をスキップ
        if (frozenAbove_) {
            // スプリングが固定状態と争わないよう、player に対する現在の camPos を維持する
            desired = camPos_;
        } else {
            // オービット操作: コントローラ右スティックまたはマウス移動から yaw/pitch を更新
            // コントローラ: 右スティック軸 (RX, RY)。マウス: 画面中心からの偏差を用いる方式
            if (controllerPresent) {
                // right stick gives -1..1 per axis; integrate over dt
                yaw_ += pad.RX * controllerYawSpeed_ * dt;
                pitch_ += -pad.RY * controllerPitchSpeed_ * dt; // invert Y
            } else {
                // マウス入力処理:
                // 画面中央を原点として常に中央からの偏差で回転量を決める（FPS 操作に近い方式）。
                // 毎フレーム SetMousePoint で中央に戻すことで、絶対座標の積算を避ける。

                const int screenW = 800; // ゲーム描画解像度に合わせる（必要なら共有定数へ移動）
                const int screenH = 600;

                int mx = 0, my = 0;
                GetMousePoint(&mx, &my);

                int centerX = screenW / 2;
                int centerY = screenH / 2;

                // 初回利用時はカーソルを中央に配置して基準を作る
                if (prevMouseX_ == -1 && prevMouseY_ == -1) {
                    SetMousePoint(centerX, centerY);
                    prevMouseX_ = centerX;
                    prevMouseY_ = centerY;
                    // 今フレームは移動量0として扱う
                    mx = centerX; my = centerY;
                }

                // 画面中央からの偏差を計算
                int dx = mx - centerX;
                int dy = my - centerY;

                // 偏差に感度を掛けて回転量を決める（ここでは既存の mouseSensitivity_ のスケールを尊重）
                yaw_ += -dx * mouseSensitivity_; // 右方向へのマウス→カメラ右回転
                pitch_ += -dy * mouseSensitivity_;

                // 次フレームも中央からの偏差を得るためにマウスを再センタリングする
                SetMousePoint(centerX, centerY);
                prevMouseX_ = centerX; prevMouseY_ = centerY;
            }

            // ピッチ角の上限・下限をクランプして視点反転を防止
            const float maxPitch = 1.4f; // ~80 degrees
            const float minPitch = -1.2f; // allow looking slightly up
            if (pitch_ > maxPitch) pitch_ = maxPitch;
            if (pitch_ < minPitch) pitch_ = minPitch;

            // compute offset from spherical coords
            float cp = cosf(pitch_);
            float sp = sinf(pitch_);
            float cy = cosf(yaw_);
            float sy = sinf(yaw_);

            VECTOR offset = VGet(orbitRadius_ * cp * sy,
                                 orbitRadius_ * sp,
                                 orbitRadius_ * cp * cy);

            // 球面座標からオフセットを計算し、プレイヤーの周りにカメラを配置
            desired = VAdd(playerPos, VGet(offset.x, offset.y + 3.0f, offset.z));
            target_ = VAdd(playerPos, VGet(0.0f, 3.0f, 0.0f));
        }
    }

    // desired がプレイヤーの基準面よりも低くなる場合、カメラが地面にめり込まないよう接近を防ぐ。
    // ここではプレイヤー基準面（groundPlane）に対する signed 距離を比較して補正を行う。
    {
        // compute signed distances to plane
        VECTOR pToPlane = VSub(playerPos, groundPoint_);
        float playerSigned = Dot(pToPlane, groundNormal_);
        VECTOR dToPlane = VSub(desired, groundPoint_);
        float desiredSigned = Dot(dToPlane, groundNormal_);
        const float minAbovePlayer = 1.8f; // keep camera at least this above player's origin when centered

        if (desiredSigned < playerSigned + minAbovePlayer) {
            // desired がプレイヤーの上方閾値より低い場合、
            // かつ半径距離を縮めるようであれば現在の半径を維持して近づかないようにする
            VECTOR curDir = VSub(camPos_, playerPos);
            VECTOR wantDir = VSub(desired, playerPos);
            float curDist = VSize(curDir);
            float wantDist = VSize(wantDir);
            if (wantDist < curDist && curDist > 0.0001f && wantDist > 0.0001f) {
                // プレイヤーからの現在の距離を維持したまま、向きを desired の方向に向ける
                VECTOR wantDirN = VScale(wantDir, 1.0f / wantDist);
                desired = VAdd(playerPos, VScale(wantDirN, curDist));
                // also nudge desired up along normal so it's at least the minimum signed height
                float newDesiredSigned = Dot(VSub(desired, groundPoint_), groundNormal_);
                if (newDesiredSigned < playerSigned + minAbovePlayer) {
                    float push = (playerSigned + minAbovePlayer) - newDesiredSigned;
                    desired.x += groundNormal_.x * push;
                    desired.y += groundNormal_.y * push;
                    desired.z += groundNormal_.z * push;
                }
            }
        }
    }

    // 単純なスプリング積分: F = -k(x - x_desired) - c v
    VECTOR displacement = VSub(camPos_, desired);
    VECTOR springF = VScale(displacement, -springStiffness_);
    VECTOR dampingF = VScale(camVel_, -springDamping_);
    VECTOR acc = VAdd(springF, dampingF);

    // integrate velocity and position
    camVel_ = VAdd(camVel_, VScale(acc, dt));
    camPos_ = VAdd(camPos_, VScale(camVel_, dt));

    // 平面との衝突/クランプ処理:
    // カメラが地面にめり込まないように、地面法線方向の signed 距離が minAboveGround 以上になるよう補正する。
    const float minAboveGround = 1.0f; // meters above ground to keep (increased from 0.5f)
    // compute signed distance from plane: d = dot(camPos - groundPoint, groundNormal)
    VECTOR camToPlane = VSub(camPos_, groundPoint_);
    float signedDist = Dot(camToPlane, groundNormal_);
    if (signedDist < minAboveGround) {
        // 地面より低ければ滑らかに押し上げる処理
        float desiredDist = minAboveGround;
        float diff = desiredDist - signedDist;
        // move camera along normal by diff smoothly
        float lerp = 1.0f - powf(0.001f, dt);
        camPos_.x += groundNormal_.x * diff * lerp;
        camPos_.y += groundNormal_.y * diff * lerp;
        camPos_.z += groundNormal_.z * diff * lerp;

        // XZ 平面上でプレイヤーの投影点へ滑らかに寄せる処理
        // まずプレイヤーの平面上の最近傍点を求める
        VECTOR playerToPlane = VSub(playerPos, groundPoint_);
        float playerDist = Dot(playerToPlane, groundNormal_);
        VECTOR playerOnPlane = VSub(playerPos, VScale(groundNormal_, playerDist));

        // 垂直方向の貫通を即座に補正し、カメラが地面より下に行かないようにする。
        float camSignedNow = Dot(VSub(camPos_, groundPoint_), groundNormal_);
        if (camSignedNow < minAboveGround) {
            float pushUp = minAboveGround - camSignedNow;
            camPos_ = VAdd(camPos_, VScale(groundNormal_, pushUp));
            camSignedNow = minAboveGround;
            // also remove any downward velocity along the normal to avoid re-penetration
            float velN = Dot(camVel_, groundNormal_);
            if (velN < 0.0f) {
                // don't kill downward velocity instantly; damp it to avoid a sudden jerk
                camVel_ = VSub(camVel_, VScale(groundNormal_, velN * 0.6f));
            }
        }

        // Project current camera position onto plane
        VECTOR camOnPlane = VSub(camPos_, VScale(groundNormal_, camSignedNow));

        // 平面上でのカメラからプレイヤーへの方向と距離を計算
        VECTOR planeDir = VSub(playerOnPlane, camOnPlane);
        float planeDist = sqrtf(planeDir.x*planeDir.x + planeDir.y*planeDir.y + planeDir.z*planeDir.z);

        if (planeDist > 0.001f) {
            VECTOR planeDirN = VScale(planeDir, 1.0f / planeDist);

            // プレイヤー方向へ滑らかに移動させるための望ましい速度を決定
            const float maxSlideSpeed = 1.6f;
            float desiredSpeed = planeDist * 0.5f;
            if (desiredSpeed < 0.02f) desiredSpeed = 0.02f;
            if (desiredSpeed > maxSlideSpeed) desiredSpeed = maxSlideSpeed;

            VECTOR desiredVelPlane = VScale(planeDirN, desiredSpeed);

            // 現在の速度から法線成分を取り除いて平面上の速度を計算
            float velAlongNormal = Dot(camVel_, groundNormal_);
            VECTOR camVelPlane = VSub(camVel_, VScale(groundNormal_, velAlongNormal));

            // Lerp current plane velocity toward desired to create a smooth ramp-up (avoids instant jumps)
            const float rampAlpha = 0.12f; // small -> smooth start
            camVelPlane.x = camVelPlane.x * (1.0f - rampAlpha) + desiredVelPlane.x * rampAlpha;
            camVelPlane.y = camVelPlane.y * (1.0f - rampAlpha) + desiredVelPlane.y * rampAlpha;
            camVelPlane.z = camVelPlane.z * (1.0f - rampAlpha) + desiredVelPlane.z * rampAlpha;

            // clamp plane speed just in case
            float spd = sqrtf(camVelPlane.x*camVelPlane.x + camVelPlane.y*camVelPlane.y + camVelPlane.z*camVelPlane.z);
            if (spd > maxSlideSpeed) camVelPlane = VScale(camVelPlane, maxSlideSpeed / spd);

            // 法線成分を保持しつつ平面上速度を再構成
            camVel_ = VAdd(VScale(groundNormal_, velAlongNormal), camVelPlane);

            // 新しい平面速度で平面上の位置を進める（小さな dt ステップ）
            camPos_ = VAdd(camPos_, VScale(camVelPlane, dt));

            // 数値誤差で地面より下に沈んでいないかを保証する追加補正
            camSignedNow = Dot(VSub(camPos_, groundPoint_), groundNormal_);
            if (camSignedNow < minAboveGround) {
                camPos_ = VAdd(camPos_, VScale(groundNormal_, (minAboveGround - camSignedNow)));
                // damp any remaining downward normal velocity
                float vn = Dot(camVel_, groundNormal_);
                if (vn < 0.0f) camVel_ = VSub(camVel_, VScale(groundNormal_, vn * 0.5f));
            }
        } else {
            // プレイヤーの真上に非常に近い場合: 平面速度を穏やかに除去し、XZ をプレイヤー上にスナップ
            float velAlongNormal = Dot(camVel_, groundNormal_);
            VECTOR camVelPlane = VSub(camVel_, VScale(groundNormal_, velAlongNormal));
            camVelPlane = VScale(camVelPlane, 0.2f);
            camVel_ = VAdd(VScale(groundNormal_, velAlongNormal), camVelPlane);
            camPos_.x = playerOnPlane.x;
            camPos_.z = playerOnPlane.z;
        }

        // 突然の停止を避けるため速度をゼロにせず滑らかに減衰させる
        camVel_.x *= 0.6f;
        camVel_.y *= 0.25f;
        camVel_.z *= 0.6f;
        // Slide camera along plane toward player to avoid digging into ground
        // (previous position-based slide replaced by velocity-based sliding)

        // スナップ判定のためにカメラの投影とプレイヤー平面点への距離を再計算
        camSignedNow = Dot(VSub(camPos_, groundPoint_), groundNormal_);
        camOnPlane = VSub(camPos_, VScale(groundNormal_, camSignedNow));
        VECTOR diffPlane = VSub(camOnPlane, playerOnPlane);
        float diffPlaneDist = sqrtf(diffPlane.x*diffPlane.x + diffPlane.y*diffPlane.y + diffPlane.z*diffPlane.z);

        const float snapThreshold = 0.3f;
        const float minAbovePlayer = 1.8f; // keep camera at least this above player's origin when centered

        if (diffPlaneDist < snapThreshold) {
            // プレイヤーの上に直接カメラを配置（一定の signed 高さを保つスナップ）
            float targetSigned = playerDist + minAbovePlayer;
            camPos_ = VAdd(playerOnPlane, VScale(groundNormal_, targetSigned));

            // ensure at least minAboveGround
            VECTOR camToPlane3 = VSub(camPos_, groundPoint_);
            float finalSigned = Dot(camToPlane3, groundNormal_);
            if (finalSigned < minAboveGround) {
                camPos_ = VAdd(camPos_, VScale(groundNormal_, minAboveGround - finalSigned));
            }

            // reduce velocities to avoid popping
            camVel_.x *= 0.05f;
            camVel_.y *= 0.01f;
            camVel_.z *= 0.05f;

            // スナップ時はプレイヤーを真下に見るようにターゲットを設定
            target_ = playerPos;

            // 真上を向いたままの不要な移動/ジャダーを防ぐためカメラを固定する
            frozenAbove_ = true;
        } else {
            // スライド中は速度を滑らかに減衰させる
            camVel_.x *= 0.8f;
            camVel_.y *= 0.5f;
            camVel_.z *= 0.8f;

            // When sliding, ensure camera looks toward player's upper body so it's aligned
            target_ = VAdd(playerPos, VGet(0.0f, 2.0f, 0.0f));

            // 以前 frozenAbove_ だった場合でも閾値以上離れたら固定を解除する
            frozenAbove_ = false;
        }
    }

    // 追加の保護: カメラがプレイヤーに非常に近い場合、プレイヤーや地面へクリッピングしないよう
    // カメラを上方へ押し上げ、軽く外側に押し出す処理
    // compute vector in plane tangent from player to camera
    VECTOR playerToCam = VSub(camPos_, playerPos);
    // remove normal component
    float pn = Dot(playerToCam, groundNormal_);
    VECTOR tangent = VSub(playerToCam, VScale(groundNormal_, pn));
    float distXZ = sqrtf(tangent.x*tangent.x + tangent.z*tangent.z + tangent.y*tangent.y); // distance along plane
    const float closeThreshold = 0.7f; // meters
    const float minAbovePlayer = 1.8f; // keep camera at least this above player's origin when centered (signed distance)
    // compute player's signed distance to plane
    VECTOR pToPlane = VSub(playerPos, groundPoint_);
    float playerSigned = Dot(pToPlane, groundNormal_);
    if (distXZ < closeThreshold) {
        // カメラがプレイヤーに近すぎる場合の高さ確保
        if ( (signedDist) < (playerSigned + minAbovePlayer) ) {
            float targetSigned = playerSigned + minAbovePlayer;
            float push = targetSigned - signedDist;
            float pushLerp = 1.0f - powf(0.001f, dt);
            camPos_.x += groundNormal_.x * push * pushLerp;
            camPos_.y += groundNormal_.y * push * pushLerp;
            camPos_.z += groundNormal_.z * push * pushLerp;
            camVel_.y *= 0.2f;

            // 接線方向に軽く外側へ押し出してプレイヤーとの接触を避ける
            float nudge = (closeThreshold - distXZ) * 0.5f;
            if (distXZ > 0.0001f) {
                camPos_.x += (tangent.x / distXZ) * -nudge;
                camPos_.y += (tangent.y / distXZ) * -nudge;
                camPos_.z += (tangent.z / distXZ) * -nudge;
            } else {
                camPos_.x += 0.01f; camPos_.y += 0.01f; camPos_.z += 0.01f;
            }
            // プレイヤーの上半身（若干上）を注視するようターゲットを調整
            target_ = VAdd(playerPos, VGet(0.0f, 2.2f, 0.0f));
        }
    }

    // frozenAbove_ の場合、位置変化を最小化してジャダーを避けるため速度を強く減衰
    if (frozenAbove_) {
        // 小さな速度をゼロにし位置をロックしてジャダーを防ぐ
        if (VSize(camVel_) < 0.01f) {
            camVel_.x = camVel_.y = camVel_.z = 0.0f;
        } else {
            // damp strongly
            camVel_.x *= 0.05f;
            camVel_.y *= 0.02f;
            camVel_.z *= 0.05f;
        }
        // lock camPos_ to current to avoid drift
        // (no changes; we've already set camPos_ earlier when snapping)
    }

    SetCameraPositionAndTarget_UpVecY(camPos_, target_);
}

// Return camera forward vector projected to XZ plane and normalized.
VECTOR CameraRig::GetForwardXZ() const
{
    VECTOR f = VSub(target_, camPos_);
    f.y = 0.0f;
    float len = sqrtf(f.x*f.x + f.z*f.z);
    if (len < 0.0001f) return VGet(0.0f, 0.0f, 1.0f);
    return VGet(f.x / len, 0.0f, f.z / len);
}

// getters
VECTOR CameraRig::GetCameraPosition() const { return camPos_; }
VECTOR CameraRig::GetForward() const { return VSub(target_, camPos_); }
