#pragma once
#include <DxLib.h>
#include <EffekseerForDXLib.h>
#include <vector>

// エフェクト管理クラス
class EffectManager
{
public:
    EffectManager();                    // コンストラクタ
    ~EffectManager();                   // デストラクタ
    void Initialize();                  // 初期化
    void Load();                        // 読み込み
    void Update(VECTOR playPosition);   // 更新
    void Draw();                        // 描画
    // Play an effect at the given world position. If filePath is nullptr, the default loaded effect is used.
    void PlayEffectAt(const VECTOR& pos, const char* filePath = nullptr);
    void PlayEffectAt(const VECTOR& pos, const char* filePath, float scale);
private:
    // 定数
    const int    EffectParticleLimit    = 20000;                // 画面に表示できる最大パーティクル数
    const char*  EffectFilePath         = "Data/Flame.efk";   // エフェクトのファイルパス
    const float  EffectSize             = 1.0f;                 // エフェクトのサイズ
    const int    EffectPlayInterval     = 300;                  // エフェクトを再生する周期
    const float  EffectMoveSpeed        = 0.2f;                 // エフェクトが移動する速度

    // 変数
    int effectResourceHandle;    // エフェクトのリソース用ハンドル
    int playingEffectHandle;     // 再生中のエフェクトハンドル

    // any extra effect resources loaded on-the-fly (deleted in destructor)
    std::vector<int> extraEffectResources;

    // 今回の動作でのみ必要な変数
    int playCount;               // 周期的に再生するためのカウント
    // debug: last spawned effect positions (cleared each frame)
    std::vector<VECTOR> debugSpawnPoints;
};
