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
                // 簡易検証のため、まずはフレーム位置に武器を置くだけの最小構成にする。
                MATRIX fm = MV1GetFrameLocalWorldMatrix(baseModelHandle_, fh);
                VECTOR framePos = VGet(fm.m[3][0], fm.m[3][1], fm.m[3][2]);
                pickedFramePos = framePos;
                havePickedFramePos = true;

                // Minimal placement: set position to frame position, apply weapon scale and no rotation.
                MV1SetPosition(wh, framePos);
                MV1SetScale(wh, VGet(spec.equipScale, spec.equipScale, spec.equipScale));
                MV1SetRotationXYZ(wh, VGet(0.0f, 0.0f, 0.0f));
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
