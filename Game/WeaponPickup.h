// WeaponPickup.h
// 概要:
//  - ワールドに落ちている武器の表現を担当する最小実装クラス
//  - 武器の種類、ワールド位置、取得済みフラグ、描画・取得判定補助を提供する
//
// 設計メモ (将来の拡張):
//  - 将来的には拾う/調べる/開ける/話す といった汎用の Interactable インターフェースに
//    統合することを想定しています。例:
//      struct Interactable {
//          std::string GetPromptText() const; // 例: "△で拾う"
//          bool CanInteract(const Player& p) const;
//          void Interact(Player& p);
//      };
//  - WeaponPickup は現在は最小実装ですが、上記の Interactable を実装する形で Event/Action 系
//    処理に統合すると、ピックアップ/会話/調査/扉といった要素を同一の扱いで扱えるようになります。
#pragma once
#include "DxLib.h"
#include "WeaponTypes.h"

class AssetsMgr; // forward

namespace Game {

class WeaponPickup
{
public:
    // assets: optional pointer to AssetsMgr to obtain shared model handles.
    // If assets is nullptr the pickup will render the legacy cross marker.
    WeaponPickup(WeaponType type = WeaponType::None, const VECTOR& pos = VGet(0,0,0), AssetsMgr* assets = nullptr);
    ~WeaponPickup();

    // Note: destructor will delete the per-pickup duplicated model instance if one
    // was successfully created via AssetsMgr::CreateWeaponModelInstance. It will not
    // delete shared cached models returned by AssetsMgr::GetWeaponModelHandle.

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
    AssetsMgr* assets_ = nullptr;
    // Per-pickup dedicated model handle (duplicated from shared model). -1 if not available.
    int modelHandle_ = -1;
    // Per-pickup transform so each pickup can be transformed independently
    // without touching shared model handles.
    float pickupScale_ = 1.0f;
    VECTOR pickupRotation_ = VGet(0.0f, 0.0f, 0.0f);
};

} // namespace Game
