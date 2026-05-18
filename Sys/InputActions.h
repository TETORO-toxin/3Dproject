#pragma once

// InputActions.h
// 目的:
//  - ゲーム内で使用する「意味ベース」の入力アクション一覧を定義します。
//  - 物理ボタン（A/B/X/Y やマウス/キー）に依存せず、ゲーム側ロジックは
//    ここで定義されたアクション名で参照できるようになります。
//  - 将来的に外部設定ファイルやキーコンフィグ画面で物理ボタンとアクション
//    を紐付けるための基準になる想定です。

// 注意: 今すぐ全てのアクションを使う必要はありません。名前だけでも揃えておく
// と後の機能拡張で崩れにくくなります。

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
    // 追加のゲーム固有アクションはここに列挙してください
};

// 補足コメント (日本語):
// - Move / Look は軸データ (float moveX/moveY / aimX/aimY) と組で使います。
// - AttackLight / AttackHeavy はそれぞれ軽攻撃/重攻撃を表す抽象アクションです。
// - Interact は近くのオブジェクトに対するコンテキストアクション（拾う/調べる等）に使われます。
// - UseLeftAction / UseSkillModifier などは補助装備やスキルのバインドを想定しています。
// - 将来的にはこの enum をキー設定 UI と結びつける実装を追加してください。
