// FrameLimiter.h
// 概要:
//  - 固定フレームレート(FPS)でループを制御するユーティリティクラス
//  - 高精度タイマー(QueryPerformanceCounter)を用いて、Sleepによる粗い待ち時間
//    とスピン待ちによる微調整を組み合わせて安定したフレームレートを実現します。
//  - どのディスプレイや解像度で動作していても、CPU時間に依存せずに同じFPSを目指します。
//
// 使い方:
//  FrameLimiter limiter(60); // 60FPS固定
//  while(...) {
//    limiter.FrameStart();   // フレーム先頭で呼ぶ
//    // 更新/描画処理
//    limiter.FrameEndWait(); // フレーム末尾で呼ぶ（次フレーム開始まで待機）
//  }

#pragma once

#include <Windows.h>

class FrameLimiter {
public:
    // fps: 目標フレームレート（例: 60）
    explicit FrameLimiter(int fps = 60);
    ~FrameLimiter();

    // フレームの開始時に呼ぶ
    void FrameStart();

    // フレームの終了時に呼んで、次フレーム開始まで待機する
    void FrameEndWait();

    // 現在設定されているFPSを取得/変更
    int GetFPS() const { return fps_; }
    void SetFPS(int fps);

private:
    int fps_;                   // 目標FPS
    LARGE_INTEGER freq_;        // パフォーマンスカウンタの周波数
    LARGE_INTEGER frameStart_;  // フレーム開始時刻の参照
    LONGLONG targetTicks_;      // 1フレームの目標ティック数
};
