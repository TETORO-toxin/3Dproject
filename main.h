#pragma once
#include "DxLib.h"
#include "App/SceneMgr.h"

class Main
{
public:
	Main() = default;
	~Main();

	/// <summary>
	/// ゲームメインの初期化処理
	/// </summary>
	void Init();
	/// <summary>
	/// ゲームメインの更新処理
	/// </summary>
	void Update();

private:
	SceneMgr* scene_ = nullptr;
};
