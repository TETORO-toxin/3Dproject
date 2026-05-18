// WeaponPickup.h
// 概要:
//  - ワールドに落ちている武器の表現を担当する最小実装クラス
//  - 武器の種類、ワールド位置、取得済みフラグ、描画・取得判定補助を提供する
#pragma once
#include "DxLib.h"
#include "WeaponTypes.h"

namespace Game {

class WeaponPickup
{
public:
    WeaponPickup(WeaponType type = WeaponType::None, const VECTOR& pos = VGet(0,0,0));

    WeaponType GetType() const { return type_; }
    VECTOR GetPosition() const { return pos_; }
    bool IsPicked() const { return picked_; }
    void MarkPicked() { picked_ = true; }

    // 指定プレイヤー位置に対して XZ 平面距離のみで取得可能か判定
    bool CanPickupBy(const VECTOR& playerPos, float range) const;

    // 簡易描画: ワールドに球を描いて武器名を表示する
    void Draw() const;

private:
    WeaponType type_;
    VECTOR pos_;
    bool picked_ = false;
};

} // namespace Game
