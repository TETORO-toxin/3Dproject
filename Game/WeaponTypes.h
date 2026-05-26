// WeaponTypes.h
// 概要:
//  - ゲーム内で扱う武器の種類を列挙し、武器ごとの見た目情報をまとめて返すヘッダです。
//  - これまで "武器種別 -> 名前 / エフェクト差分" が散在していたため、
//    武器の表示名・モデルパス・スケール・座標/回転補正・エフェクト情報などを
//    1 つの構造体 `WeaponSpec` に集約し、`GetWeaponSpec(WeaponType)` で取得できるようにします。
//
// 目的:
//  - モデルのピックアップ表示（地面に落ちているモデル）と装備表示（プレイヤーに装備した状態）の
//    見た目差分を武器固有で管理しやすくする。
//  - 攻撃エフェクトに関するスケール/オフセット/専用ファイル/色などの設定も武器ごとに持てるようにする。
//
// 設計方針（M1 要件）:
//  - `WeaponSpec` に以下のような情報を持たせる:
//      - 表示名
//      - Pickup（地面上表示）用モデルパス
//      - 装備（プレイヤー装備）用モデルパス
//      - Pickup 表示スケール / 回転（モデル原点や向きが異なるため補正）
//      - Equip 表示スケール / オフセット / 回転（プレイヤーボーンに合わせるための補正）
//      - 攻撃エフェクトのスケール / オフセット / 専用エフェクトファイルパス / 色
//  - 将来の拡張を見越して、色は DxLib の `GetColor` 形式の `unsigned int` で保持する。
//
// 使用例:
//  const WeaponSpec& spec = Game::GetWeaponSpec(Game::WeaponType::IronPipe);
//  sprite->SetScale(spec.equipScale);
//  effectManager.Play(spec.effectFile ? spec.effectFile : defaultFile, spec.effectOffset, spec.effectScale, spec.effectColor);

#pragma once

#include "DxLib.h"

namespace Game {

enum class WeaponType {
    None = 0,
    IronPipe,
};

// WeaponSpec:
//  - 武器の見た目に関わる設定をまとめた構造体です。
//  - 各フィールドは下記の目的で使用します。
//
// displayName: ユーザー向け表示名（UI やログで使用）
// pickupModelPath: 地面に置かれた武器アイテム用のモデルファイルパス。nullptr は未設定。
// equipModelPath: プレイヤーに装備した際に使用するモデルファイルパス。nullptr は未設定。
// pickupScale: Pickup 表示時のスケール（1.0 = モデルの原寸）
// pickupRotation: Pickup 表示時の回転補正（ローカル空間、単位は度。VGet を用いて格納）
// equipScale: 装備表示時のスケール
// equipOffset: 装備表示時の位置補正（プレイヤー基準のローカル座標: x=左右, y=高さ, z=前方）
// equipRotation: 装備表示時の回転補正（ローカル、単位は度）
// effectScale: 攻撃エフェクトのスケール（1.0 = デフォルト）
// effectOffset: 攻撃エフェクト出現オフセット（プレイヤー基準ローカル座標）
// effectFile: 武器専用のエフェクトファイルパス。nullptr の場合は EffectManager のデフォルトを使用
// effectColor: エフェクトに付与したい色（DxLib の GetColor 形式）。将来 Effekseer パラメータに渡せるように保持。
struct WeaponSpec {
    const char* displayName;

    // モデル関連
    const char* pickupModelPath;
    const char* equipModelPath;

    // Pickup 表示補正
    float pickupScale;
    VECTOR pickupRotation; // Euler degrees, use VGet()

    // 装備表示補正
    float equipScale;
    VECTOR equipOffset;    // プレイヤー基準ローカル座標
    VECTOR equipRotation;  // Euler degrees, use VGet()

    // 攻撃エフェクト関連
    float effectScale;
    VECTOR effectOffset;
    const char* effectFile;
    unsigned int effectColor;
};

// GetWeaponSpec:
//  - 指定した WeaponType に対応する WeaponSpec を返します。
//  - ここでは関数内の static 変数として各武器の定義を持たせ、1 箇所で管理できるようにしています。
//  - 新しい武器を追加する場合はこの関数の中にエントリを追加してください。
inline const WeaponSpec& GetWeaponSpec(WeaponType t)
{
    // 関数内 static にすることで初期化順序問題を回避します。
    static const WeaponSpec none = {
        "なし",    // displayName
        nullptr,    // pickupModelPath
        nullptr,    // equipModelPath
        1.0f,       // pickupScale
        VGet(0.0f, 0.0f, 0.0f), // pickupRotation
        1.0f,       // equipScale
        VGet(0.0f, 0.0f, 0.0f), // equipOffset
        VGet(0.0f, 0.0f, 0.0f), // equipRotation
        0.7f,       // effectScale (素手/未装備は控えめ)
        VGet(0.0f, 1.1f, 0.9f), // effectOffset
        nullptr,    // effectFile
        GetColor(255, 255, 255), // effectColor
    };

    static const WeaponSpec ironPipe = {
        "鉄パイプ", // displayName (日本語)
        // pickupModelPath: 将来アセットを配置したら実ファイルパスを指定する
        "assets/models/RustyPipe_Normal_DirectX.mv1",
        // equipModelPath: 装備表示用モデル
        "assets/models/RustyPipe_Normal_DirectX.mv1",
        // pickupScale: 地面上の見た目を少し大きめに
        0.05f,
        // pickupRotation: モデル原点の向きに合わせて回転補正（度）
        VGet(0.0f, 90.0f, 90.0f),
        // equipScale: 右手フレーム基準でのスケール。フレームに対するローカル補正として扱います。
        0.05f,
        // equipOffset: 右手フレーム基準のローカル座標。x=左右, y=高さ, z=前方
        VGet(0.0f, 0.0f, 0.0f),
        // equipRotation: 装備時に向きを合わせるための回転補正（右手フレーム基準、度）
        VGet(0.0f, 90.0f, 90.0f),
        // effectScale: 鉄パイプはやや大きめのエフェクト
        1.2f,
        // effectOffset: エフェクト出現位置（プレイヤー基準ローカル）
        VGet(0.0f, 1.0f, 1.4f),
        // effectFile: 将来的に専用エフェクトがあればここにパスを設定
        nullptr,
        // effectColor: 金属っぽい色味
        GetColor(200, 180, 140),
    };

    switch (t) {
    case WeaponType::IronPipe: return ironPipe;
    case WeaponType::None:
    default: return none;
    }
}

// 互換性補助: 既存のコードが GetWeaponName を使っている場合のために残す
inline const char* GetWeaponName(WeaponType t)
{
    return GetWeaponSpec(t).displayName;
}

} // namespace Game

