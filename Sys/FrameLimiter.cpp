// FrameLimiter.cpp
// 概要:
//  - 固定フレームレート(FPS)でループを制御する実装ファイル
//  - 高精度タイマー(QueryPerformanceCounter)を用いて、Sleepによる粗い待ち時間
//    とスピン待ちによる微調整を組み合わせて安定したフレームレートを実現します。
//  - どのディスプレイや解像度で動作していても、CPU時間に依存せずに同じFPSを目指します。
//
// 詳細:
//  - FrameStart() をフレーム先頭で呼び、FrameEndWait() をフレーム末尾で呼ぶことで
//    次フレームまでの待機を行います。
//  - Sleep は精度が粗いため、残り時間が十分にあるときにのみ使い、最後は短時間の
//    スピン待ちで正確なタイミング合わせを行います。

#include "FrameLimiter.h"
#include <assert.h>

FrameLimiter::FrameLimiter(int fps)
    : fps_(fps)
{
    assert(fps_ > 0);
    QueryPerformanceFrequency(&freq_);
    targetTicks_ = freq_.QuadPart / fps_;
    frameStart_.QuadPart = 0;
}

FrameLimiter::~FrameLimiter()
{
}

void FrameLimiter::FrameStart()
{
    QueryPerformanceCounter(&frameStart_);
}

void FrameLimiter::FrameEndWait()
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    LONGLONG elapsed = now.QuadPart - frameStart_.QuadPart;
    LONGLONG remain = targetTicks_ - elapsed;
    if (remain <= 0) {
        // 既にフレーム時間を超過している場合は待たない
        return;
    }

    // 残り時間をミリ秒に変換
    double remainMs = (double)remain * 1000.0 / (double)freq_.QuadPart;

    // 残りが十分にある場合は Sleep で大まかな待ちを行う（精度向上のため1ms引く）
    if (remainMs > 2.0) {
        DWORD ms = (DWORD)(remainMs) - 1;
        if (ms > 0) Sleep(ms);
    }

    // スピン待ちで精密合わせ
    while (true) {
        QueryPerformanceCounter(&now);
        elapsed = now.QuadPart - frameStart_.QuadPart;
        if (elapsed >= targetTicks_) break;
        // ここであえて短時間のヒントを与える（0msスリープ）
        Sleep(0);
    }
}

void FrameLimiter::SetFPS(int fps)
{
    if (fps <= 0) return;
    fps_ = fps;
    targetTicks_ = freq_.QuadPart / fps_;
}
