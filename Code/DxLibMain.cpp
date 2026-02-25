#include "DxLib.h"
#include "../main.h"
#include <Windows.h>

//プロトタイプ宣言
void UpdateGameMain();
int InitDxLib();

int InitDxLib()
{
    // Choose monitor to fullscreen: use the monitor containing the mouse cursor.
    POINT pt;
    if (GetCursorPos(&pt)) {
        HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfo(hMon, &mi)) {
            int monitorW = mi.rcMonitor.right - mi.rcMonitor.left;
            int monitorH = mi.rcMonitor.bottom - mi.rcMonitor.top;
            // Set DxLib graph mode to the monitor resolution and request fullscreen exclusive
            SetGraphMode(monitorW, monitorH, 32);
            ChangeWindowMode(FALSE); // fullscreen
        } else {
            // fallback to primary display size if monitor info failed
            int pw = GetSystemMetrics(SM_CXSCREEN);
            int ph = GetSystemMetrics(SM_CYSCREEN);
            SetGraphMode(pw, ph, 32);
            ChangeWindowMode(FALSE);
        }
    } else {
        // fallback to primary display
        int pw = GetSystemMetrics(SM_CXSCREEN);
        int ph = GetSystemMetrics(SM_CYSCREEN);
        SetGraphMode(pw, ph, 32);
        ChangeWindowMode(FALSE);
    }

    // DxLib の初期化（失敗時は終了）
    if (DxLib_Init() == -1)
        return 0;


    // 背景色を暗めに設定（夜っぽい雰囲気）
    SetBackgroundColor(0, 0, 20);

    // 裏画面を使用（ダブルバッファリング）
    SetDrawScreen(DX_SCREEN_BACK);
    return 1;
}

// アプリケーションのエントリーポイント（WinMain関数）
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    //DxLibの初期化処理を呼び出し
    InitDxLib();

    //ゲーム関連の処理を呼び出し
    UpdateGameMain();

    // DxLibの終了処理
    DxLib_End();
    return 0;
}


void UpdateGameMain()
{
    Main mainInstance;
    mainInstance.Init();

    //エスケープキーが押されるまで処理を繰り返す
    while (CheckHitKey(KEY_INPUT_ESCAPE) == false)
    {
        // 画面をリセットするDXライブラリの処理
        ClearDrawScreen();

        mainInstance.Update();

        // 画面に表示するDXライブラリの処理
        ScreenFlip();
    }
}

