#pragma once

struct SupportStatus {
    bool awakening = false;
    float time = 0.0f; // seconds remaining
};

inline void ActivateAwakening(SupportStatus& s, float duration = 8.0f)
{
    s.awakening = true;
    s.time = duration;
}

inline void TickSupport(SupportStatus& s, float dt)
{
    if (s.awakening) {
        s.time -= dt;
        if (s.time <= 0.0f) {
            s.awakening = false;
            s.time = 0.0f;
        }
    }
}
