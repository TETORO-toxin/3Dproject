// WeaponTypes.h
// 概要:
//  - ゲーム内で扱う武器の種類を列挙するヘッダです。
//  - 将来の拡張を見越して補助関数もここに用意しています。
//  - 現在は最小実装: None / IronPipe
#pragma once

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

// 将来用: エフェクトスタイルなどを返すプレースホルダ
inline int GetWeaponEffectStyle(WeaponType /*t*/) { return 0; }

} // namespace Game
