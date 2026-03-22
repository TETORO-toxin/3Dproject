// Player.cpp
// サマリー:
//  - もともと1ファイルにまとまっていたPlayerの実装を役割ごとに分割しました。
//  - 実際の実装は次の分割ファイルに移動しています:
//      - Player_Init.cpp       : コンストラクタ / デストラクタ / リソース初期化
//      - Player_Animation.cpp  : アニメーションの再生・更新・イベント
//      - Player_InputAndMovement.cpp : 入力処理・移動・ジャンプ・物理
//      - Player_Draw.cpp       : 描画ロジック（フェード・モデル描画・プレースホルダ）
//      - Player_AuxAndCombat.cpp : 補助装備（Aux）・被弾/無敵/回復等
//  - ここには最小限の実装だけを残し、他ファイルの関数を呼ぶラッパーを提供します。

#include "Player.h"

// 下位互換: Updateはロジック->描画を呼ぶ
void Player::Update()
{
    UpdateLogic();
    Draw();
}
