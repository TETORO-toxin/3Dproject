#include "DxLib.h"
#include "EffectManager.h"
#include "DebugPrint.h"
#include <cstdlib>
#include <cmath>

// コンストラクタ
EffectManager::EffectManager()
    : effectResourceHandle  (-1)
    , playingEffectHandle   (-1)
    , playCount             (0)
{
    // 初期化
    Initialize();

    // 読み込み
    Load();
}

// デストラクタ
EffectManager::~EffectManager()
{
    // エフェクトリソースの開放
    // (Effekseer終了時に破棄されるので削除しなくてもいい)
    DeleteEffekseerEffect(effectResourceHandle);
}

// 初期化
void EffectManager::Initialize()
{
    // DirectX11を使用するようにする。(DirectX9も可、一部機能不可)
    // Effekseerを使用するには必ず設定する。
    SetUseDirect3DVersion(DX_DIRECT3D_11);

    // 引数には画面に表示する最大パーティクル数を設定する。
    if (Effkseer_Init(EffectParticleLimit) == -1) {
        initialized_ = false;
        effectResourceHandle = -1;
        return; // 絶対に DxLib_End() しない
    }

    initialized_ = true;
    // フルスクリーンウインドウの切り替えでリソースが消えるのを防ぐ。
    // Effekseerを使用する場合は必ず設定する。
    SetChangeScreenModeGraphicsSystemResetFlag(FALSE);

    // DXライブラリのデバイスロストした時のコールバックを設定する。
    // ウインドウとフルスクリーンの切り替えが発生する場合は必ず実行する。
    Effekseer_SetGraphicsDeviceLostCallbackFunctions();

    // Zバッファを有効にする。
    // Effekseerを使用する場合、2DゲームでもZバッファを使用する。
    SetUseZBuffer3D(TRUE);

    // Zバッファへの書き込みを有効にする。
    // Effekseerを使用する場合、2DゲームでもZバッファを使用する。
    SetWriteZBuffer3D(TRUE);
    // 3D設定をEffekseerに同期する
    Effekseer_Sync3DSetting();

    // seed random for randomized effect rotations
    srand((unsigned int)GetNowCount());
}

// 読み込み
void EffectManager::Load()
{
    // エフェクトのリソースを読み込む
    effectResourceHandle = LoadEffekseerEffect(EffectFilePath, EffectSize);
    DebugPrint("Effect load handle=%d path=%s\n", effectResourceHandle, EffectFilePath);
}

/// <summary>
/// 更新
/// </summary>
/// <param name="playPosition">再生座標</param>
void EffectManager::Update(VECTOR playPosition, VECTOR forward)
{
    if (!initialized_) return;

    // Do not auto-play effects here. Effects are played explicitly via PlayEffectAt
    // (for example when the player attacks). Only update the positions of any
    // currently playing effect and advance Effekseer.
    if (playingEffectHandle != -1) {
        // place effect slightly above and in front of the player based on forward vector
        float fx = forward.x;
        float fy = forward.y;
        float fz = forward.z;
        float fl = sqrtf(fx*fx + fy*fy + fz*fz);
        if (fl > 1e-6f) {
            fx /= fl; fy /= fl; fz /= fl;
        } else {
            // fallback: forward along +Z
            fx = 0.0f; fy = 0.0f; fz = 1.0f;
        }
        float px = playPosition.x + fx * EffectForwardOffset;
        float py = playPosition.y + EffectVerticalOffset;
        float pz = playPosition.z + fz * EffectForwardOffset;
        SetPosPlayingEffekseer3DEffect(playingEffectHandle, px, py, pz);
    }

    // Effekseerにより再生中のエフェクトを更新する。
    UpdateEffekseer3D();
}

// 描画
void EffectManager::Draw()
{
    if (!initialized_) return;
    // Effekseerにより再生中のエフェクトを描画する。
    // 毎フレームカメラや投影行列を変更するタイプのゲームでは、
    // 描画直前に3D設定を同期しておく。
    Effekseer_Sync3DSetting();
    DrawEffekseer3D();
}

// Simple PlayEffectAt implementations to satisfy header (optional usage)
void EffectManager::PlayEffectAt(const VECTOR& pos, const char* filePath)
{
    if (!initialized_) return;
    PlayEffectAt(pos, filePath, 1.0f);
}

void EffectManager::PlayEffectAt(const VECTOR& pos, const char* filePath, float scale)
{
    if (!initialized_) return;
    int res = effectResourceHandle;
    if (filePath) {
        int h = LoadEffekseerEffect(filePath, EffectSize * scale);
        if (h != -1) {
            extraEffectResources.push_back(h);
            res = h;
        }
    }
    if (res == -1) return;
    int ph = PlayEffekseer3DEffect(res);
    if (ph != -1) {
        // remember handle so Update can move it each frame
        playingEffectHandle = ph;
        // apply vertical offset so effect appears above the player
        SetPosPlayingEffekseer3DEffect(ph, pos.x, pos.y + EffectVerticalOffset, pos.z);
        // set a random rotation so each spawned effect has a different orientation
        float yaw = (static_cast<float>(rand()) / RAND_MAX) * DX_PI_F * 2.0f; // 0..2pi
        // small random pitch/roll tilt (+/- ~0.2 rad)
        float tiltRange = 0.4f;
        float pitch = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * tiltRange;
        float roll  = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * tiltRange;
        SetRotationPlayingEffekseer3DEffect(ph, pitch, yaw, roll);
        // slow down playback so effect lasts longer (1.0 = normal speed)
        SetSpeedPlayingEffekseer3DEffect(ph, EffectPlaybackSpeed);
        // debug: record spawn point (with offset)
        debugSpawnPoints.push_back(VGet(pos.x, pos.y + EffectVerticalOffset, pos.z));
    }
}
