#pragma once
#include "DxLib.h"
#include <cmath>
#include "InputActions.h"

// XInput / Joypad helper with deadzone normalization and simple helpers
struct PadState {
    XINPUT_STATE xi{}; // raw XInput state
    int          pad;  // DXLib pad bitflags (GetJoypadInputState)

    // normalized axes and triggers (range -1..1 for sticks, 0..1 for triggers)
    float LX = 0.0f, LY = 0.0f, RX = 0.0f, RY = 0.0f;
    float LT = 0.0f, RT = 0.0f;
};

template<typename T>
inline T clamp(const T& v, const T& lo, const T& hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Poll pad with optional padIndex (0 = PAD1) and thumb deadzone (default = 7849, same as XInput)
inline PadState PollPad(int padIndex = 0, int deadzone = 7849)
{
    PadState s;
    int dxPad = DX_INPUT_PAD1 + padIndex;
    GetJoypadXInputState(dxPad, &s.xi);
    s.pad = GetJoypadInputState(dxPad);

    auto normThumb = [&](short v) -> float {
        int iv = static_cast<int>(v);
        if (std::abs(iv) <= deadzone) return 0.0f;
        float sign = iv < 0 ? -1.0f : 1.0f;
        float mag = (std::abs(iv) - deadzone) / (32767.0f - deadzone);
        return clamp(mag, 0.0f, 1.0f) * sign;
    };

    s.LX = normThumb(s.xi.ThumbLX);
    s.LY = normThumb(s.xi.ThumbLY);
    s.RX = normThumb(s.xi.ThumbRX);
    s.RY = normThumb(s.xi.ThumbRY);
    s.LT = clamp(s.xi.LeftTrigger / 255.0f, 0.0f, 1.0f);
    s.RT = clamp(s.xi.RightTrigger / 255.0f, 0.0f, 1.0f);

    return s;
}

// Helpers for raw pad
inline bool IsButtonDown(const PadState& s, int padMask) { return (s.pad & padMask) != 0; }
inline float LeftStickX(const PadState& s) { return s.LX; }
inline float LeftStickY(const PadState& s) { return s.LY; }
inline float RightStickX(const PadState& s) { return s.RX; }
inline float RightStickY(const PadState& s) { return s.RY; }
inline float LeftTrigger(const PadState& s) { return s.LT; }
inline float RightTrigger(const PadState& s) { return s.RT; }

// Unified input state combining controller and keyboard+mouse
//
// この構造体は2つの目的を兼ねます:
//  1) アナログ的な入力値 (移動やエイム、トリガー) を保持する
//  2) 意味ベースのアクション状態 (押下中 / 押下開始 / リリース) を保持する
//
// 将来的に入力処理をアクション中心に扱うための基盤で、物理的なボタン名
// (btnA/btnB 等) は後方互換のために残してあります。
struct InputState {
    // movement -1..1
    float moveX = 0.0f;
    float moveY = 0.0f;
    // aim -1..1 (screen-space normalized)
    float aimX = 0.0f;
    float aimY = 0.0f;
    // triggers
    float leftTrigger = 0.0f;
    float rightTrigger = 0.0f;
    // buttons
    bool btnA = false; // action / switch
    bool btnB = false; // dodge
    bool btnX = false;
    bool btnY = false;
    bool mouseLeft = false;
    bool mouseRight = false;

    // --- 将来拡張用: D-Pad / 十字キー (意味的ボタンとして予約) ---
    // 目的: D-pad を武器スロット切替やアイテム操作に割り当てる想定。
    // このフラグは PollInput で設定され、SceneMgr 等が解釈して処理を行います。
    // Left/Right は左右の武器スロット切替、Up はアイテム使用、Down はアイテム切替等に割当予定。
    bool dpadLeftDown = false;   // ← 押下中
    bool dpadRightDown = false;  // → 押下中
    bool dpadUpDown = false;     // ↑ 押下中
    bool dpadDownDown = false;   // ↓ 押下中

    // --- 将来拡張用: modifier (修飾ボタン) の予約 ---
    // 例: L1/L2/R1/R2 を modifier として持ち、SceneMgr や CombatController 側で解釈して
    // 例えば "L2 + face button" のような複合入力を実現します。
    bool modifierL1 = false;
    bool modifierL2 = false;
    bool modifierR1 = false;
    bool modifierR2 = false;

    // --- 意味ベースのアクションフラグ ---
    // Down = 押下中 (毎フレーム)
    // Pressed = 今フレーム押下が開始された (エッジ検出)
    // Released = 今フレーム離された
    // 将来的には holdTime 等も追加し、長押し判定に対応できるようにする予定
    bool interactDown = false;    // 汎用コンテキスト操作 (拾う / 調べる 等)
    bool interactPressed = false;
    bool interactReleased = false;

    // --- 長押し対応: 各アクションの hold 時間を保存 ---
    // PollInput はフレーム単位で呼ばれる前提なので、ここでは秒単位の累積時間を保持します。
    // 長押し判定 (例: R2 長押しで溜め攻撃) は SceneMgr / CombatController 側で holdTime を参照して行います。
    float interactHoldTime = 0.0f;    // 秒
    float attackLightHoldTime = 0.0f; // 秒
    float attackHeavyHoldTime = 0.0f; // 秒
    float leftTriggerHoldTime = 0.0f; // 秒 (アナログトリガー用)
    float rightTriggerHoldTime = 0.0f; // 秒

    bool jumpDown = false;
    bool jumpPressed = false;
    bool jumpReleased = false;

    bool dodgeDown = false;
    bool dodgePressed = false;
    bool dodgeReleased = false;

    bool attackLightDown = false;
    bool attackLightPressed = false;
    bool attackLightReleased = false;

    bool attackHeavyDown = false;
    bool attackHeavyPressed = false;
    bool attackHeavyReleased = false;
};

// Poll unified input. Uses controller if present, otherwise keyboard+mouse.
// Poll unified input. Uses controller if present, otherwise keyboard+mouse.
//
// ここでは物理入力 (キー/パッド) を「ゲーム内アクション」へマッピングし、
// 前フレーム状態との差分から Pressed / Released を算出します。
// PollInput はフレームごとに1回だけ呼ぶことを想定しています。
inline InputState PollInput(int padIndex = 0)
{
    InputState out;

    PadState pad = PollPad(padIndex);
    // Detect controller presence by checking any significant input or pad flags
    bool controllerPresent = (pad.pad != 0) || (pad.xi.ThumbLX != 0) || (pad.xi.ThumbLY != 0) || (pad.xi.ThumbRX != 0) || (pad.xi.ThumbRY != 0) || (pad.xi.LeftTrigger != 0) || (pad.xi.RightTrigger != 0);

    if (controllerPresent) {
        out.moveX = pad.LX;
        out.moveY = pad.LY;
        out.aimX = pad.RX;
        out.aimY = pad.RY;
        out.leftTrigger = pad.LT;
        out.rightTrigger = pad.RT;
        out.btnA = IsButtonDown(pad, PAD_INPUT_1);
        out.btnB = IsButtonDown(pad, PAD_INPUT_2);
        out.btnX = IsButtonDown(pad, PAD_INPUT_3);
        out.btnY = IsButtonDown(pad, PAD_INPUT_4);
        // D-pad を意味的なフラグとして設定 (将来の武器切替等に利用予定)
        out.dpadLeftDown = IsButtonDown(pad, PAD_INPUT_LEFT);
        out.dpadRightDown = IsButtonDown(pad, PAD_INPUT_RIGHT);
        out.dpadUpDown = IsButtonDown(pad, PAD_INPUT_UP);
        out.dpadDownDown = IsButtonDown(pad, PAD_INPUT_DOWN);

        // 修飾ボタンの暫定マッピング: 物理ボタンに割当てる想定
        out.modifierL1 = IsButtonDown(pad, PAD_INPUT_5); // 暫定: L1
        out.modifierR1 = IsButtonDown(pad, PAD_INPUT_6); // 暫定: R1
        // L2/R2 はトリガーを修飾として扱うことも多いため modifierL2/modifierR2 はトリガー値閾値で解釈される想定
    } else {
        // keyboard WASD / arrows for movement
        float mx = 0.0f, my = 0.0f;
        if (CheckHitKey(KEY_INPUT_D) || CheckHitKey(KEY_INPUT_RIGHT)) mx += 1.0f;
        if (CheckHitKey(KEY_INPUT_A) || CheckHitKey(KEY_INPUT_LEFT)) mx -= 1.0f;
        if (CheckHitKey(KEY_INPUT_W) || CheckHitKey(KEY_INPUT_UP)) my -= 1.0f; // up reduces y for screen coords
        if (CheckHitKey(KEY_INPUT_S) || CheckHitKey(KEY_INPUT_DOWN)) my += 1.0f;
        // normalize
        float mag = std::hypot(mx, my);
        if (mag > 1.0f) { mx /= mag; my /= mag; }
        out.moveX = mx;
        out.moveY = my;

        // mouse aiming: convert mouse pos to -1..1 around screen centre
        int mxp = 0, myp = 0;
        GetMousePoint(&mxp, &myp);
        const int screenW = 800, screenH = 600; // match TitleScene default; adapt if needed
        out.aimX = clamp((mxp - screenW * 0.5f) / (screenW * 0.5f), -1.0f, 1.0f);
        out.aimY = clamp((myp - screenH * 0.5f) / (screenH * 0.5f), -1.0f, 1.0f);

        int mouseInput = GetMouseInput();
        out.mouseLeft = (mouseInput & MOUSE_INPUT_LEFT) != 0;
        out.mouseRight = (mouseInput & MOUSE_INPUT_RIGHT) != 0;

        // map keyboard buttons (物理->意味の暫定マッピング)
        // Jump should be triggered by Space or Z
        out.btnA = CheckHitKey(KEY_INPUT_SPACE) || CheckHitKey(KEY_INPUT_Z);
        out.btnB = CheckHitKey(KEY_INPUT_X);
        out.btnX = CheckHitKey(KEY_INPUT_C); // 仮: C を X 相当に割当（ジャンプ等の代替）
        out.btnY = CheckHitKey(KEY_INPUT_E); // 仮: E を Y 相当に割当（インタラクト）
        out.leftTrigger = out.mouseLeft ? 1.0f : 0.0f;
        out.rightTrigger = out.mouseRight ? 1.0f : 0.0f;

        // Keyboard による D-pad 的操作も予約 (左/右/上下矢印キー)
        out.dpadLeftDown = CheckHitKey(KEY_INPUT_LEFT);
        out.dpadRightDown = CheckHitKey(KEY_INPUT_RIGHT);
        out.dpadUpDown = CheckHitKey(KEY_INPUT_UP);
        out.dpadDownDown = CheckHitKey(KEY_INPUT_DOWN);

        // 修飾キーの暫定例: Shift/Ctrl を modifier として扱う (将来はコントローラとの統合を行う)
        out.modifierL1 = (GetKeyState(KEY_INPUT_LSHIFT) & 0x8000) != 0;
        out.modifierR1 = (GetKeyState(KEY_INPUT_RSHIFT) & 0x8000) != 0;
    }

    // ---------------------------
    // Action mapping (意味ベース)
    // ---------------------------
    // 暫定のマッピング: 将来的に Sys/InputActions.h を使って外部設定可能にする
    // Move / Look はアナログ値をそのまま利用

    // Jump: controller A / keyboard (Space or Z)
    // Avoid using C here to prevent conflicts with other keyboard mappings (C is mapped to btnX).
    bool jumpDown = out.btnA;
    // Dodge: controller B / keyboard X
    bool dodgeDown = out.btnB;
    // AttackLight: controller X or mouse left
    bool attackLightDown = out.btnX || out.mouseLeft;
    // AttackHeavy: right trigger or mouse right
    bool attackHeavyDown = (out.rightTrigger > 0.5f) || out.mouseRight;
    // Interact: Y or E
    bool interactDown = out.btnY || CheckHitKey(KEY_INPUT_E);

    // Static previous state to compute pressed/released edges. PollInput is expected to be
    // called once per frame, so this simple static approach works for now.
    static InputState prev{};

    out.jumpDown = jumpDown;
    out.jumpPressed = jumpDown && !prev.jumpDown;
    out.jumpReleased = !jumpDown && prev.jumpDown;

    out.dodgeDown = dodgeDown;
    out.dodgePressed = dodgeDown && !prev.dodgeDown;
    out.dodgeReleased = !dodgeDown && prev.dodgeDown;

    out.attackLightDown = attackLightDown;
    out.attackLightPressed = attackLightDown && !prev.attackLightDown;
    out.attackLightReleased = !attackLightDown && prev.attackLightDown;

    out.attackHeavyDown = attackHeavyDown;
    out.attackHeavyPressed = attackHeavyDown && !prev.attackHeavyDown;
    out.attackHeavyReleased = !attackHeavyDown && prev.attackHeavyDown;

    out.interactDown = interactDown;
    out.interactPressed = interactDown && !prev.interactDown;
    out.interactReleased = !interactDown && prev.interactDown;

    // --- Hold time 更新 (簡易実装: フレーム長は60FPS 想定で 1/60 秒を加算) ---
    // 将来的には外部から正確な dt を渡して更新する方針が望ましい。
    const float assumedDt = 1.0f / 60.0f;
    out.interactHoldTime = prev.interactHoldTime + (out.interactDown ? assumedDt : 0.0f);
    out.attackLightHoldTime = prev.attackLightHoldTime + (out.attackLightDown ? assumedDt : 0.0f);
    out.attackHeavyHoldTime = prev.attackHeavyHoldTime + (out.attackHeavyDown ? assumedDt : 0.0f);
    out.leftTriggerHoldTime = prev.leftTriggerHoldTime + (out.leftTrigger > 0.001f ? assumedDt : 0.0f);
    out.rightTriggerHoldTime = prev.rightTriggerHoldTime + (out.rightTrigger > 0.001f ? assumedDt : 0.0f);

    // Save some of the action-raw mapping back into compatibility fields
    out.btnA = jumpDown; // keep semantic relation
    out.btnB = dodgeDown;

    // store for next frame
    prev = out;

    return out;
}

// Convenience: convert InputState to Pad-like queries for legacy code
inline float InputLeftX() { return PollInput().moveX; }
inline float InputLeftY() { return PollInput().moveY; }
