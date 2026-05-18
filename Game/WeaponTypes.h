// WeaponTypes.h
// 概要:
//  - ゲーム内で扱う武器の種類を列挙するヘッダです。
//  - モーションは共通化したまま、武器ごとに攻撃エフェクトの差分（ファイル/スケール/出現位置/色）を返す
//    ヘルパーを提供します。現状は最小実装で "None" と "IronPipe" の差分を用意しています。
#pragma once

#include "DxLib.h"

namespace Game {

enum class WeaponType {
    None = 0,
    IronPipe,
};

// 武器の表示名を返すヘルパー
inline const char* GetWeaponName(WeaponType t)
{
    switch (t) {
    case WeaponType::IronPipe: return "鉄パイプ"; // 日本語表示
    case WeaponType::None:
    default: return "なし";
    }
}

// --- 以下、武器ごとのエフェクト差分を返す簡易ヘルパー ---
// 目的: モーション共通で武器に応じた視覚差分だけ変えられるようにする
// 実装方針:
//  - スケールはエフェクトの大きさ (1.0 = デフォルト)
//  - オフセットはプレイヤー位置に対する相対位置 (x=左右, y=高さ, z=前方)
//  - ファイルは武器専用のエフェクトファイルがあればそのパスを返す。なければ nullptr を返して EffectManager のデフォルトを使う
//  - 色は将来 Effekseer のパラメータを変更する際に使うための値（現状は EffectManager で色指定に未対応）

// エフェクトのスケール (1.0 = デフォルト)
inline float GetWeaponEffectScale(WeaponType t)
{
    switch (t) {
    case WeaponType::IronPipe: return 1.2f; // 鉄パイプはやや大きめ
    case WeaponType::None:
    default: return 0.7f; // 素手/未装備は控えめ
    }
}

// エフェクト出現オフセット (ローカル)
inline VECTOR GetWeaponEffectOffset(WeaponType t)
{
    switch (t) {
    case WeaponType::IronPipe: return VGet(0.0f, 1.0f, 1.4f); // すこし前方かつやや低め
    case WeaponType::None:
    default: return VGet(0.0f, 1.1f, 0.9f); // 素手は前方小さめ
    }
}

// 専用エフェクトファイルのパス。nullptr の場合は EffectManager のデフォルトを使用
inline const char* GetWeaponEffectFile(WeaponType t)
{
    switch (t) {
    case WeaponType::IronPipe:
        // 将来的に専用エフェクトがあればここにパスを返す。
        // 例: return "Assets/VFX/Slash/PipeSlash.efk";
        return nullptr;
    case WeaponType::None:
    default:
        return nullptr;
    }
}

// 武器のエフェクト色（DxLib の GetColor 形式）
// 注: 現在 EffectManager では色を直接適用する仕組みがないため将来拡張用の値です。
inline unsigned int GetWeaponEffectColor(WeaponType t)
{
    switch (t) {
    case WeaponType::IronPipe: return GetColor(200, 180, 140); // 金属っぽい色味
    case WeaponType::None:
    default: return GetColor(255, 255, 255);
    }
}

} // namespace Game

