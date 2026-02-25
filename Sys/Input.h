#pragma once
#include "DxLib.h"
#include <cmath>

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
};

// Poll unified input. Uses controller if present, otherwise keyboard+mouse.
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

        // map keyboard buttons
        out.btnA = CheckHitKey(KEY_INPUT_SPACE) || CheckHitKey(KEY_INPUT_Z);
        out.btnB = CheckHitKey(KEY_INPUT_X);
        out.leftTrigger = out.mouseLeft ? 1.0f : 0.0f;
        out.rightTrigger = out.mouseRight ? 1.0f : 0.0f;
    }

    return out;
}

// Convenience: convert InputState to Pad-like queries for legacy code
inline float InputLeftX() { return PollInput().moveX; }
inline float InputLeftY() { return PollInput().moveY; }
