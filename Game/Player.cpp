// Player.cpp
// 概要:
//  - 以前1つにまとまっていた Player 実装を責務ごとに分割しました。
//  - 実装は以下のファイルに分かれています:
//      - Player_Init.cpp       : コンストラクタ / デストラクタ / リソース初期化
//      - Player_Animation.cpp  : アニメーションの再生・更新・イベント
//      - Player_InputAndMovement.cpp : 入力処理・移動・ジャンプ・物理
//      - Player_Draw.cpp       : 描画ロジック（フェード・モデル描画・プレースホルダ）
//      - Player_AuxAndCombat.cpp : 補助装備（Aux）・被弾/無敵/回復など
//  - このファイルには最低限のラッパー実装のみを置き、各責務ファイルを呼び出します。

#include "Player.h"

// 下位互換: Updateはロジック->描画を呼ぶ
void Player::Update()
{
    UpdateLogic();
    Draw();
}
