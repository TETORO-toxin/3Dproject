// Player_Draw.cpp
// サマリー:
//  - 描画に関する処理を集約したファイルです。
//  - カメラとの相対位置に基づくフェード判定、モデルのアルファブレンド、モデル/プレースホルダの描画、
//    及び簡易UI文字列の描画を担当します。

#include "Player.h"
#include "CameraRig.h"
#include "../Sys/DebugPrint.h"
#include "../Sys/Assets.h"
#include <cmath>

void Player::Draw()
{
    // カメラ距離に基づいてフェードアルファを計算し、カメラがプレイヤー内部に入り込むのを回避
    int drawAlpha = 255;
    if (camera_ != nullptr) {
        VECTOR camPos = camera_->GetCameraPosition();

        // 簡易貫通判定: カメラがプレイヤーのローカルバウンディングボックス内なら完全に隠す
        float halfX = 0.6f * visualScale_; // 半幅
        float halfZ = 0.6f * visualScale_; // 半奥行
        float height = 1.6f * visualScale_; // 高さ

        float dx = camPos.x - x_;
        float dy = camPos.y - y_;
        float dz = camPos.z - z_;

        bool insideBox = (fabsf(dx) <= halfX * 0.9f) && (fabsf(dz) <= halfZ * 0.9f) && (dy >= 0.0f) && (dy <= height * 1.05f);
        if (insideBox) {
            drawAlpha = 0;
        } else {
            VECTOR toCam = VSub(camPos, GetPosition());
            float dist = VSize(toCam);
            const float fadeStart = 2.0f; // フェード開始距離
            const float fadeEnd = 0.6f;   // 完全透明になる距離
            if (dist < fadeStart) {
                float t = (dist - fadeEnd) / (fadeStart - fadeEnd);
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
                drawAlpha = (int)(255.0f * t + 0.5f);
            }
        }
    }

    // モデル/プリミティブ描画にアルファブレンドを適用
    DxLib::SetDrawBlendMode(DX_BLENDMODE_ALPHA, drawAlpha);

    // モデルを描画
    // アクティブなモデルハンドルを優先して描画する。MV1AttachModelが利用できない場合のフォールバックでは
    // 以前は`modelHandle_`をアニメモデルに差し替えていたため、最初に`baseModelHandle_`を描画するとフォールバックが見えなくなっていた。
    // コンストラクタで`modelHandle_`はベースに初期化されているため、これを主要な描画ハンドルとして使う。
    if (modelHandle_ != -1)
    {
        // Restore transform (position/scale/rotation) before drawing the model so
        // the MV1 model is placed correctly in world space.
        VECTOR pos = VGet(x_, y_, z_);
        ::MV1SetPosition(modelHandle_, pos);
        ::MV1SetScale(modelHandle_, VGet(visualScale_, visualScale_, visualScale_));
        // Apply yaw rotation (currentYaw_ is in radians)
        // Model's forward vector is rotated 180 degrees relative to yaw; add PI to match orientation.
        ::MV1SetRotationXYZ(modelHandle_, VGet(0.0f, currentYaw_ + DX_PI_F, 0.0f));
        ::MV1DrawModel(modelHandle_); // グローバル名前空間を明示
    }
    else if (baseModelHandle_ != -1)
    {
        // Ensure base model also has correct transform applied
        VECTOR pos = VGet(x_, y_, z_);
        ::MV1SetPosition(baseModelHandle_, pos);
        ::MV1SetScale(baseModelHandle_, VGet(visualScale_, visualScale_, visualScale_));
        ::MV1SetRotationXYZ(baseModelHandle_, VGet(0.0f, currentYaw_ + DX_PI_F, 0.0f));
        ::MV1DrawModel(baseModelHandle_);
    }
    // 装備武器の描画は下部で一元的に行うためここでは何もしない
    else
    {
        // プレースホルダ: ワールド上の角を投影してプレイヤーの周りに3Dキューブを描く
        float halfX = 0.6f * visualScale_; // 半幅
        float halfZ = 0.6f * visualScale_; // 半奥行
        float height = 1.6f * visualScale_; // 高さ
        VECTOR base = VGet(x_, y_, z_);
        VECTOR corners[8];
        // 下面
        corners[0] = VAdd(base, VGet(-halfX, 0.0f, -halfZ));
        corners[1] = VAdd(base, VGet( halfX, 0.0f, -halfZ));
        corners[2] = VAdd(base, VGet( halfX, 0.0f,  halfZ));
        corners[3] = VAdd(base, VGet(-halfX, 0.0f,  halfZ));
        // 上面
        corners[4] = VAdd(base, VGet(-halfX, height, -halfZ));
        corners[5] = VAdd(base, VGet( halfX, height, -halfZ));
        corners[6] = VAdd(base, VGet( halfX, height,  halfZ));
        corners[7] = VAdd(base, VGet(-halfX, height,  halfZ));

        VECTOR scr[8];
        for (int i = 0; i < 8; ++i) scr[i] = ConvWorldPosToScreenPos(corners[i]);

        auto drawEdge = [&](int a, int b, unsigned int color){
            if (scr[a].z > 0.0f && scr[b].z > 0.0f) {
                DrawLine((int)scr[a].x, (int)scr[a].y, (int)scr[b].x, (int)scr[b].y, color);
            }
        };

        unsigned int colFill = GetColor(100,100,255);
        unsigned int colEdge = GetColor(255,255,255);
        // エッジ: 下面
        drawEdge(0,1,colEdge); drawEdge(1,2,colEdge); drawEdge(2,3,colEdge); drawEdge(3,0,colEdge);
        // エッジ: 上面
        drawEdge(4,5,colEdge); drawEdge(5,6,colEdge); drawEdge(6,7,colEdge); drawEdge(7,4,colEdge);
        // 縦線
        drawEdge(0,4,colEdge); drawEdge(1,5,colEdge); drawEdge(2,6,colEdge); drawEdge(3,7,colEdge);
    }

    // ブレンドを解除してその後のUI/テキスト描画に影響しないようにする
    DxLib::SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    // --- 装備武器の描画: 右手フレームに追従する ---
    bool usedFollow = false;
    // Prepare diagnostic info for follow/fallback
    int fh = rightHandFrameIndex_;
    // Resolved weapon model handle used for placement; keep for diagnostics
    int whDiag = equippedWeaponModelHandle_;
    VECTOR pickedFramePos = VGet(0.0f, 0.0f, 0.0f);
    bool havePickedFramePos = false;
    const char* followReason = "";
    const char* pathStr = "Fallback";
    if (equippedWeapon_ != Game::WeaponType::None) {
        // make weapon handle available for diagnostic checks after this block
        int wh = whDiag;
        // Try to obtain model handle via AssetsMgr if not cached
        if (wh == -1 && assets_) {
            wh = assets_->GetWeaponModelHandle(equippedWeapon_, /*equip=*/true);
        }

        // update diagnostic copy
        whDiag = wh;

        if (wh != -1) {
            const Game::WeaponSpec& spec = Game::GetWeaponSpec(equippedWeapon_);
            // Use frame API unconditionally to get the frame world matrix
            if (fh != -1 && baseModelHandle_ != -1) {
                // entering follow branch
                usedFollow = true;
                pathStr = "Follow";
                // MV1GetFrameLocalWorldMatrix を使ってフレームのワールド行列を取得
                // Some DxLib builds provide a 2-arg variant that returns MATRIX,
                // adjust call to match the available signature.
                MATRIX fm = MV1GetFrameLocalWorldMatrix(baseModelHandle_, fh);
                VECTOR framePos = VGet(fm.m[3][0], fm.m[3][1], fm.m[3][2]);
                pickedFramePos = framePos;
                havePickedFramePos = true;
                // フレームの基底ベクトルからオフセットを変換
                VECTOR right = VGet(fm.m[0][0], fm.m[0][1], fm.m[0][2]);
                VECTOR up = VGet(fm.m[1][0], fm.m[1][1], fm.m[1][2]);
                VECTOR forward = VGet(fm.m[2][0], fm.m[2][1], fm.m[2][2]);

                // Remove any scale encoded in the frame basis by normalizing the basis
                // vectors. This ensures weapon world matrix does not inherit player's
                // scale (avoids double-scaling when frame matrix already includes it).
                auto safeNormalize = [](VECTOR v)->VECTOR{
                    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
                    if (len > 1e-6f) return VGet(v.x/len, v.y/len, v.z/len);
                    return VGet(0.0f, 0.0f, 0.0f);
                };
                VECTOR nRight = safeNormalize(right);
                VECTOR nUp = safeNormalize(up);
                VECTOR nForward = safeNormalize(forward);
                // Build a frame matrix without scale
                MATRIX fmNoScale = fm;
                fmNoScale.m[0][0] = nRight.x; fmNoScale.m[0][1] = nRight.y; fmNoScale.m[0][2] = nRight.z; fmNoScale.m[0][3] = 0.0f;
                fmNoScale.m[1][0] = nUp.x;    fmNoScale.m[1][1] = nUp.y;    fmNoScale.m[1][2] = nUp.z;    fmNoScale.m[1][3] = 0.0f;
                fmNoScale.m[2][0] = nForward.x; fmNoScale.m[2][1] = nForward.y; fmNoScale.m[2][2] = nForward.z; fmNoScale.m[2][3] = 0.0f;
                fmNoScale.m[3][0] = framePos.x; fmNoScale.m[3][1] = framePos.y; fmNoScale.m[3][2] = framePos.z; fmNoScale.m[3][3] = 1.0f;

                // Build local offset and rotate it by weapon local rotation (equipRotation)
                // Note: do NOT multiply offsets/scales by `visualScale_` here. The frame matrix `fm`
                // returned by MV1GetFrameLocalWorldMatrix may already include the player's scale.
                // Weapon sizing is controlled solely by `spec.equipScale` and weapon-local offsets
                // are taken from `spec.equipOffset` in world units relative to the frame.
                VECTOR localOff = VGet(spec.equipOffset.x, spec.equipOffset.y, spec.equipOffset.z);

                // Convert equip rotation (degrees) to radians
                float d2r = DX_PI_F / 180.0f;
                VECTOR rotDeg = spec.equipRotation; // degrees
                float rx = rotDeg.x * d2r;
                float ry = rotDeg.y * d2r;
                float rz = rotDeg.z * d2r;

                // Rotate localOff by Euler angles (X -> Y -> Z) in local weapon space
                auto rotateLocal = [&](const VECTOR& v)->VECTOR{
                    float cx = std::cos(rx), sx = std::sin(rx);
                    float cy = std::cos(ry), sy = std::sin(ry);
                    float cz = std::cos(rz), sz = std::sin(rz);

                    // Apply X
                    VECTOR a;
                    a.x = v.x;
                    a.y = v.y * cx - v.z * sx;
                    a.z = v.y * sx + v.z * cx;

                    // Apply Y
                    VECTOR b;
                    b.x = a.x * cy + a.z * sy;
                    b.y = a.y;
                    b.z = -a.x * sy + a.z * cy;

                    // Apply Z
                    VECTOR c;
                    c.x = b.x * cz - b.y * sz;
                    c.y = b.x * sz + b.y * cz;
                    c.z = b.z;
                    return c;
                };

                VECTOR localOffRot = rotateLocal(localOff);

                // Build local transform matrix (translation = localOffRot, rotation = equipRotation, scale = equipScale*visualScale_)
                auto MakeLocalMatrix = [&](const VECTOR& translationLocal, float rx_, float ry_, float rz_, float scale)->MATRIX{
                    MATRIX m;
                    // Rotation matrices
                    float cx = std::cos(rx_), sx = std::sin(rx_);
                    float cy = std::cos(ry_), sy = std::sin(ry_);
                    float cz = std::cos(rz_), sz = std::sin(rz_);

                    // R = Rz * Ry * Rx (applies Rx then Ry then Rz)
                    float R00 = (cz * cy);
                    float R01 = (cz * sy * sx - sz * cx);
                    float R02 = (cz * sy * cx + sz * sx);

                    float R10 = (sz * cy);
                    float R11 = (sz * sy * sx + cz * cx);
                    float R12 = (sz * sy * cx - cz * sx);

                    float R20 = -sy;
                    float R21 = cy * sx;
                    float R22 = cy * cx;

                    // Apply scale to rotation basis
                    float s = scale;
                    m.m[0][0] = R00 * s; m.m[0][1] = R01 * s; m.m[0][2] = R02 * s; m.m[0][3] = 0.0f;
                    m.m[1][0] = R10 * s; m.m[1][1] = R11 * s; m.m[1][2] = R12 * s; m.m[1][3] = 0.0f;
                    m.m[2][0] = R20 * s; m.m[2][1] = R21 * s; m.m[2][2] = R22 * s; m.m[2][3] = 0.0f;

                    m.m[3][0] = translationLocal.x; m.m[3][1] = translationLocal.y; m.m[3][2] = translationLocal.z; m.m[3][3] = 1.0f;
                    return m;
                };

                // Multiply matrices: C = A * B
                auto MulM = [&](const MATRIX& A, const MATRIX& B)->MATRIX{
                    MATRIX C;
                    for (int r = 0; r < 4; ++r) {
                        for (int c = 0; c < 4; ++c) {
                            float sum = 0.0f;
                            for (int k = 0; k < 4; ++k) sum += A.m[r][k] * B.m[k][c];
                            C.m[r][c] = sum;
                        }
                    }
                    return C;
                };

                MATRIX localMat = MakeLocalMatrix(localOffRot, rx, ry, rz, spec.equipScale);
                // Final world matrix = frameMatrix_without_scale * localMatrix
                MATRIX finalMat = MulM(fmNoScale, localMat);
                MV1SetMatrix(wh, finalMat);
                ::MV1DrawModel(wh);
            } else {
                // フォールバック: プレイヤー位置からの相対オフセットで配置
                // Use spec.equipOffset directly (world-relative) and spec.equipScale for weapon size.
                VECTOR wpos = VAdd(VGet(x_, y_, z_), VGet(spec.equipOffset.x, spec.equipOffset.y, spec.equipOffset.z));
                // when not following a frame, record the fallback weapon position for diagnostics
                pickedFramePos = wpos;
                havePickedFramePos = true;
                MV1SetPosition(wh, wpos);
                MV1SetScale(wh, VGet(spec.equipScale, spec.equipScale, spec.equipScale));
                float d2r = DX_PI_F / 180.0f;
                VECTOR rotDeg = spec.equipRotation;
                MV1SetRotationXYZ(wh, VGet(rotDeg.x * d2r, rotDeg.y * d2r, rotDeg.z * d2r));
                ::MV1DrawModel(wh);
            }
        }
    }

    // Determine follow failure reason if not using follow
    if (!usedFollow) {
        if (fh == -1) followReason = "fh==-1";
        else if (baseModelHandle_ == -1) followReason = "baseModelHandle==-1";
        else if (
            // if there was no equipped weapon or weapon model could not be resolved
            (equippedWeapon_ != Game::WeaponType::None && /*equipped but model unresolved*/ true)
        ) followReason = "wh==-1";
        else followReason = "unknown";
        pathStr = "Fallback";
    } else {
        followReason = "";
    }

    // show some debug info: current right-hand frame index, frame name, follow path/reason, frame world pos, whether we used follow, and equipped weapon model handle
    DrawFormatString(10, 30, GetColor(255, 255, 255), "Mode: %s  Just:%s  AuxGauge: %.1f  HP: %.0f",
        mode_==Mode::Melee?"Melee":"Ranged", justExecuted_?"Yes":"No", auxGauge, hp);

    DrawFormatString(10, 50, GetColor(255,255,255), "RHFrameIdx: %d  RHFrameName: %s  Path: %s  FollowUsed: %s  WpnHandle: %d",
        rightHandFrameIndex_, rightHandFrameName_.c_str(), pathStr, usedFollow?"Y":"N", whDiag);

    DrawFormatString(10, 70, GetColor(255,255,0), "FollowFailReason: %s",
        followReason);

    if (havePickedFramePos) {
        DrawFormatString(10, 90, GetColor(200,255,200), "PickedFramePos: (%.2f, %.2f, %.2f)", pickedFramePos.x, pickedFramePos.y, pickedFramePos.z);
    } else {
        DrawFormatString(10, 90, GetColor(200,255,200), "PickedFramePos: n/a");
    }
}
