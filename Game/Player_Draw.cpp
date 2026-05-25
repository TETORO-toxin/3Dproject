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
        ::MV1DrawModel(modelHandle_); // グローバル名前空間を明示
    }
    else if (baseModelHandle_ != -1)
    {
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
    if (equippedWeapon_ != Game::WeaponType::None) {
        int wh = equippedWeaponModelHandle_;
        // Try to obtain model handle via AssetsMgr if not cached
        if (wh == -1 && assets_) {
            wh = assets_->GetWeaponModelHandle(equippedWeapon_, /*equip=*/true);
        }

        if (wh != -1) {
            const Game::WeaponSpec& spec = Game::GetWeaponSpec(equippedWeapon_);
#ifdef MV1GetFrameLocalWorldMatrix
            int fh = rightHandFrameIndex_;
            if (fh != -1 && baseModelHandle_ != -1) {
                // MV1GetFrameLocalWorldMatrix を使ってフレームのワールド行列を取得
                MATRIX fm;
                MV1GetFrameLocalWorldMatrix(baseModelHandle_, fh, &fm);
                VECTOR framePos = VGet(fm.m[3][0], fm.m[3][1], fm.m[3][2]);
                // フレームの基底ベクトルからオフセットを変換
                VECTOR right = VGet(fm.m[0][0], fm.m[0][1], fm.m[0][2]);
                VECTOR up = VGet(fm.m[1][0], fm.m[1][1], fm.m[1][2]);
                VECTOR forward = VGet(fm.m[2][0], fm.m[2][1], fm.m[2][2]);

                // Build local offset and rotate it by weapon local rotation (equipRotation)
                // Build local offset and rotated offset by equip rotation
                VECTOR localOff = VGet(spec.equipOffset.x * visualScale_, spec.equipOffset.y * visualScale_, spec.equipOffset.z * visualScale_);

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

                MATRIX localMat = MakeLocalMatrix(localOffRot, rx, ry, rz, spec.equipScale * visualScale_);
                // Final world matrix = frameMatrix * localMatrix
                MATRIX finalMat = MulM(fm, localMat);
                MV1SetMatrix(wh, &finalMat);
                ::MV1DrawModel(wh);
            } else
#endif
            {
                // フォールバック: プレイヤー位置からの相対オフセットで配置
                VECTOR wpos = VAdd(VGet(x_, y_, z_), VGet(spec.equipOffset.x * visualScale_, spec.equipOffset.y * visualScale_, spec.equipOffset.z * visualScale_));
                MV1SetPosition(wh, wpos);
                MV1SetScale(wh, VGet(spec.equipScale * visualScale_, spec.equipScale * visualScale_, spec.equipScale * visualScale_));
                float d2r = DX_PI_F / 180.0f;
                VECTOR rotDeg = spec.equipRotation;
                MV1SetRotationXYZ(wh, VGet(rotDeg.x * d2r, rotDeg.y * d2r, rotDeg.z * d2r));
                ::MV1DrawModel(wh);
            }
        }
    }

    DrawFormatString(10, 30, GetColor(255, 255, 255), "Mode: %s  Just:%s  AuxGauge: %.1f  HP: %.0f", mode_==Mode::Melee?"Melee":"Ranged", justExecuted_?"Yes":"No", auxGauge, hp);
}
