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
#include <cstring>

#ifdef MV1SetMatrix
#undef MV1SetMatrix
#endif

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
                // 簡易検証のため、まずはフレーム位置に武器を置くだけの最小構成にする。
                MATRIX fm = MV1GetFrameLocalWorldMatrix(baseModelHandle_, fh);
                VECTOR framePos = VGet(fm.m[3][0], fm.m[3][1], fm.m[3][2]);
                pickedFramePos = framePos;
                havePickedFramePos = true;

                // Extract frame basis (assume rows 0..2 are right/up/forward)
                auto getRow = [&](int r)->VECTOR{
                    return VGet(fm.m[r][0], fm.m[r][1], fm.m[r][2]);
                };
                VECTOR right = getRow(0);
                VECTOR up = getRow(1);
                VECTOR forward = getRow(2);

                // Normalize basis vectors to use only rotation (remove scale)
                auto normalize = [](VECTOR v)->VECTOR{
                    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
                    if (len > 1e-6f) return VGet(v.x/len, v.y/len, v.z/len);
                    return VGet(0.0f,0.0f,0.0f);
                };
                // cross product helper
                auto cross = [](const VECTOR &a, const VECTOR &b)->VECTOR{
                    return VGet(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
                };

                right = normalize(right);
                up = normalize(up);
                forward = normalize(forward);

                // Remap axes for weapon-friendly orientation.
                // Example: weapon forward should face -forward, and weapon right should be -right.
                VECTOR wForward = VGet(-forward.x, -forward.y, -forward.z);
                VECTOR wRight   = VGet(-right.x,   -right.y,   -right.z);
                // Compute a consistent up axis by orthonormalizing the basis.
                VECTOR wUp = cross(wForward, wRight);

                wForward = normalize(wForward);
                wUp = normalize(wUp);
                // Recompute right to ensure strict orthogonality: right = up x forward
                wRight = cross(wUp, wForward);
                wRight = normalize(wRight);

                // Build a cleaned hand-frame matrix (row-major, translation in row 3)
                MATRIX hand = fm; // start from frame matrix
                // overwrite basis with remapped, orthonormal axes (preserve translation)
                hand.m[0][0] = wRight.x; hand.m[0][1] = wRight.y; hand.m[0][2] = wRight.z; hand.m[0][3] = 0.0f;
                hand.m[1][0] = wUp.x;    hand.m[1][1] = wUp.y;    hand.m[1][2] = wUp.z;    hand.m[1][3] = 0.0f;
                hand.m[2][0] = wForward.x;hand.m[2][1] = wForward.y;hand.m[2][2] = wForward.z;hand.m[2][3] = 0.0f;

                // Build local transform from equipRotation and equipOffset and scale
                float d2r = DX_PI_F / 180.0f;
                float rx = spec.equipRotation.x * d2r;
                float ry = spec.equipRotation.y * d2r;
                float rz = spec.equipRotation.z * d2r;

                // Create local rotation matrices (row-major)
                auto makeRotX = [&](float a)->MATRIX{
                    MATRIX m = {};
                    // row-major
                    m.m[0][0] = 1.0f; m.m[0][1]=0; m.m[0][2]=0; m.m[0][3]=0;
                    m.m[1][0] = 0; m.m[1][1]=cosf(a); m.m[1][2]=sinf(a); m.m[1][3]=0;
                    m.m[2][0] = 0; m.m[2][1]=-sinf(a); m.m[2][2]=cosf(a); m.m[2][3]=0;
                    m.m[3][0]=0; m.m[3][1]=0; m.m[3][2]=0; m.m[3][3]=1.0f;
                    return m;
                };
                auto makeRotY = [&](float a)->MATRIX{
                    MATRIX m = {};
                    m.m[0][0]=cosf(a); m.m[0][1]=0; m.m[0][2]=-sinf(a); m.m[0][3]=0;
                    m.m[1][0]=0; m.m[1][1]=1.0f; m.m[1][2]=0; m.m[1][3]=0;
                    m.m[2][0]=sinf(a); m.m[2][1]=0; m.m[2][2]=cosf(a); m.m[2][3]=0;
                    m.m[3][0]=0; m.m[3][1]=0; m.m[3][2]=0; m.m[3][3]=1.0f;
                    return m;
                };
                auto makeRotZ = [&](float a)->MATRIX{
                    MATRIX m = {};
                    m.m[0][0]=cosf(a); m.m[0][1]=sinf(a); m.m[0][2]=0; m.m[0][3]=0;
                    m.m[1][0]=-sinf(a); m.m[1][1]=cosf(a); m.m[1][2]=0; m.m[1][3]=0;
                    m.m[2][0]=0; m.m[2][1]=0; m.m[2][2]=1.0f; m.m[2][3]=0;
                    m.m[3][0]=0; m.m[3][1]=0; m.m[3][2]=0; m.m[3][3]=1.0f;
                    return m;
                };

                // Compose local rotation: apply X, then Y, then Z -> R = Rz * Ry * Rx
                MATRIX Rx = makeRotX(rx);
                MATRIX Ry = makeRotY(ry);
                MATRIX Rz = makeRotZ(rz);

                auto mulMat = [&](const MATRIX &a, const MATRIX &b)->MATRIX{
                    MATRIX out = {};
                    for (int r = 0; r < 4; ++r) {
                        for (int c = 0; c < 4; ++c) {
                            float s = 0.0f;
                            for (int k = 0; k < 4; ++k) s += a.m[r][k] * b.m[k][c];
                            out.m[r][c] = s;
                        }
                    }
                    return out;
                };

                MATRIX localR = mulMat(Rz, mulMat(Ry, Rx));

                // local translation = equipOffset (in hand-local coords)
                MATRIX localT = localR;
                // apply uniform scale to rotation axes
                float sc = spec.equipScale;
                for (int r=0;r<3;++r){
                    for (int c=0;c<3;++c) localT.m[r][c] *= sc;
                }
                localT.m[0][3] = spec.equipOffset.x;
                localT.m[1][3] = spec.equipOffset.y;
                localT.m[2][3] = spec.equipOffset.z;
                localT.m[3][0]=0; localT.m[3][1]=0; localT.m[3][2]=0; localT.m[3][3]=1.0f;

                // final matrix = hand * localT
                MATRIX finalMat = mulMat(hand, localT);

                // Ensure translation matches frame position (small numerical safety)
                finalMat.m[3][0] = hand.m[3][0] + finalMat.m[0][3];
                finalMat.m[3][1] = hand.m[3][1] + finalMat.m[1][3];
                finalMat.m[3][2] = hand.m[3][2] + finalMat.m[2][3];

                // Use MV1SetMatrix to set full transform: copy into DxLib::MATRIX then call
                DxLib::MATRIX finalDx;
                std::memcpy(&finalDx, &finalMat, sizeof(finalDx));
                ::MV1SetMatrix(wh, finalDx);
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
                // As above, compose player's yaw with per-weapon rotation instead of using
                // frame axes so fallback placement keeps consistent orientation with player.
                float d2r = DX_PI_F / 180.0f;
                VECTOR rotDeg = spec.equipRotation;
                float rotXfb = rotDeg.x * d2r;
                float rotYfb = currentYaw_ + rotDeg.y * d2r;
                float rotZfb = rotDeg.z * d2r;
                MV1SetRotationXYZ(wh, VGet(rotXfb, rotYfb, rotZfb));
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
