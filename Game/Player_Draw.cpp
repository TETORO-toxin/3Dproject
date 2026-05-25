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
        MV1SetPosition(modelHandle_, VGet(x_, y_, z_));
        // 均一スケールを適用
        MV1SetScale(modelHandle_, VGet(visualScale_, visualScale_, visualScale_));
        MV1DrawModel(modelHandle_);
    }
    else if (baseModelHandle_ != -1)
    {
        MV1SetPosition(baseModelHandle_, VGet(x_, y_, z_));
        // 均一スケールを適用
        MV1SetScale(baseModelHandle_, VGet(visualScale_, visualScale_, visualScale_));
        MV1DrawModel(baseModelHandle_);
    }
    // 装備武器の描画: WeaponSpec の補正を使って手元に描く（簡易実装）
    // 右手ボーンに正確に取り付ける代わりに、プレイヤー位置に対して equipOffset を加算して描画します。
    using namespace Game;
    if (GetEquippedWeapon() != WeaponType::None) {
        int wh = -1;
        // Try to use cached handle if present
        // equippedWeaponModelHandle_ may be managed elsewhere; fall back to AssetsMgr if available
        // (assets_ is a member initialized in Player ctor)
        if (/* member exists */ false) {}
        // The class exposes equippedWeaponModelHandle_ at runtime; attempt to use it via this-> (safe if present)
        // Use pointer-to-member trick not needed: attempt to read the member if it exists in this translation unit.
        // Simpler: ask AssetsMgr for the equip model handle when available.
        if (assets_) {
            wh = assets_->GetWeaponModelHandle(GetEquippedWeapon(), /*equip=*/true);
        }
            if (wh != -1) {
                const WeaponSpec& spec = GetWeaponSpec(GetEquippedWeapon());
                // If we have a valid right-hand frame on the base model, transform the equipOffset
                // by the frame's basis vectors so the weapon follows the bone orientation.
                int fh = rightHandFrameIndex_;
                if (fh != -1 && baseModelHandle_ != -1) {
                    MATRIX fm = MV1GetFrameLocalWorldMatrix(baseModelHandle_, fh);
                    VECTOR framePos = VGet(fm.m[3][0], fm.m[3][1], fm.m[3][2]);
                    // Rows 0..2 contain the frame's local axes in world space
                    VECTOR right = VGet(fm.m[0][0], fm.m[0][1], fm.m[0][2]);
                    VECTOR up = VGet(fm.m[1][0], fm.m[1][1], fm.m[1][2]);
                    VECTOR forward = VGet(fm.m[2][0], fm.m[2][1], fm.m[2][2]);

                    VECTOR localOff = VGet(spec.equipOffset.x * visualScale_, spec.equipOffset.y * visualScale_, spec.equipOffset.z * visualScale_);
                    VECTOR rotatedOff = VAdd(VAdd(VScale(right, localOff.x), VScale(up, localOff.y)), VScale(forward, localOff.z));
                    VECTOR wpos = VAdd(framePos, rotatedOff);

                    MV1SetPosition(wh, wpos);

                    float s = spec.equipScale * visualScale_;
                    MV1SetScale(wh, VGet(s, s, s));

                    // Apply the frame yaw to weapon rotation so it follows facing direction.
                    float baseYaw = std::atan2(forward.x, forward.z); // radians
                    float deg2rad = DX_PI_F / 180.0f;
                    VECTOR rotDeg = spec.equipRotation;
                    MV1SetRotationXYZ(wh, VGet(rotDeg.x * deg2rad, rotDeg.y * deg2rad + baseYaw, rotDeg.z * deg2rad));

                    MV1DrawModel(wh);
                } else {
                    // Fallback: simple position offset from player origin (existing behavior)
                    VECTOR wpos = VAdd(VGet(x_, y_, z_), VGet(spec.equipOffset.x * visualScale_, spec.equipOffset.y * visualScale_, spec.equipOffset.z * visualScale_));
                    MV1SetPosition(wh, wpos);
                    float s = spec.equipScale * visualScale_;
                    MV1SetScale(wh, VGet(s, s, s));
                    float deg2rad = DX_PI_F / 180.0f;
                    VECTOR rotDeg = spec.equipRotation;
                    MV1SetRotationXYZ(wh, VGet(rotDeg.x * deg2rad, rotDeg.y * deg2rad, rotDeg.z * deg2rad));
                    MV1DrawModel(wh);
                }
            }
    }
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
    if (equippedWeapon_ != Game::WeaponType::None && equippedWeaponModelHandle_ != -1) {
#ifdef MV1GetFrameLocalWorldMatrix
        int fh = rightHandFrameIndex_;
        if (fh != -1) {
            MATRIX m;
            MV1GetFrameLocalWorldMatrix(baseModelHandle_, fh, &m);

            // 武器の描画位置をフレームのワールド位置にオフセットで加える簡易実装
            // 将来的に回転やスケールもフレームに合わせて適用する実装に差し替える
            const Game::WeaponSpec& spec = Game::GetWeaponSpec(equippedWeapon_);
            // DxLib の MATRIX のワールド位置は m.m[3][0..2] にある想定
            VECTOR framePos = VGet(m.m[3][0], m.m[3][1], m.m[3][2]);
            VECTOR worldPos = VAdd(framePos, spec.equipOffset);

            MV1SetPosition(equippedWeaponModelHandle_, worldPos);
            MV1SetScale(equippedWeaponModelHandle_, VGet(spec.equipScale, spec.equipScale, spec.equipScale));
            // 回転は装備補正のみ適用（単純）
            float d2r = DX_PI_F / 180.0f;
            VECTOR rotDeg = spec.equipRotation;
            MV1SetRotationXYZ(equippedWeaponModelHandle_, VGet(rotDeg.x * d2r, rotDeg.y * d2r, rotDeg.z * d2r));
            MV1DrawModel(equippedWeaponModelHandle_);
        }
#endif
    }

    DrawFormatString(10, 30, GetColor(255, 255, 255), "Mode: %s  Just:%s  AuxGauge: %.1f  HP: %.0f", mode_==Mode::Melee?"Melee":"Ranged", justExecuted_?"Yes":"No", auxGauge, hp);
}
