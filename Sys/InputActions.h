#pragma once

// InputActions.h
// 目的:
//  - ゲーム内で使用する「意味ベース」の入力アクション一覧を定義します。
//  - 物理ボタン（A/B/X/Y やマウス/キー）に依存せず、ゲーム側ロジックは
//    ここで定義されたアクション名で参照できるようになります。
//  - 将来的に外部設定ファイルやキーコンフィグ画面で物理ボタンとアクション
//    を紐付けるための基準になる想定です。

enum class InputAction {
    Move,
    Look,
    Jump,
    Dodge,
    Sprint,
    AttackLight,
    AttackHeavy,
    UseLeftAction,
    UseSkillModifier,
    UseArtsModifier,
    Interact,
    ToggleHud,
    ItemUse,
    ItemNext,
    ItemPrev,
    WeaponLeftCycle,
    WeaponRightCycle,
    LockOn,
    CameraReset,
    // 追加のゲーム固有アクションはここに列挙
};

