#pragma once
#include "DxLib.h"
#include <array>
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include "../Sys/Input.h"
#include "WeaponTypes.h"

// 右手フレーム探索候補名（優先順）
// モデル差し替え時にここだけを編集すればよいように一覧にまとめる。
static const char* const kRightHandFrameCandidates[] = {
    // 優先順: モデルの違いに応じてここを変更すればよい
    "WeaponAttach_R",
    "Hand_R",
    "R_Hand",
    "RightHand",
    "r_hand",
    "hand_r",
    "hand.R",
    "手_右",
    "Right",
    "右手",
    nullptr
};

class AssetsMgr;
class ProjectileManager;
struct AuxUnit;
class CameraRig;

class Player
{
public:
    enum class Mode { Melee, Ranged };
    enum class AnimLayer { Full = 0, Lower = 1, Upper = 2 };

    Player(AssetsMgr* assets = nullptr);
    ~Player();
    void Update();

    // ロジック更新と描画を分離: シーン側で描画順を制御できるようにする
    // dt: delta time in seconds
    // 入力状態は外部で1回だけ Poll して渡す設計に変更しました。
    // これにより SceneMgr がフレーム単位で入力を一元的に取得・配布でき、
    // プレイヤー／UI／ピックアップなどで入力の競合を制御しやすくなります。
    void UpdateLogic(float dt, const InputState& in);
    void Draw();

    VECTOR GetPosition() const;

    // Scene/Camera がカメラ情報を通知して、カメラ基準の移動/向き設定に利用できるようにする
    void SetCamera(CameraRig* cam);

    // 戦闘関連 API
    void OnIncomingAttack(); // notify an incoming attack for just-dodge timing
    bool IsInvulnerable() const;

    // Aux units
    void UpdateAux(float dt, ProjectileManager& pm);
    void FireAuxLeft(ProjectileManager& pm);
    void FireAuxRight(ProjectileManager& pm);

    // Support/Status
    void SetAwakened(bool v);
    void Heal(float amount);
    float GetHP() const { return hp; }

    // アニメーション（互換性のためアニメごとに別MV1モデルを使用）
    // 注意: アクティブなモデルハンドルを差し替えるのではなく、元のベースモデルに対してアニメを適用して再生する。
    // layer パラメータは全身再生か下半身/上半身のみかを選択する。
    void PlayAnimation(const std::string& name, bool loop = false, AnimLayer layer = AnimLayer::Full);
    void UpdateAnimation(float dt);

    // アニメーションイベントヘルパー: 指定秒数に達したらコールバックを呼ぶ
    void AddAnimEvent(const std::string& animName, float timeSec, std::function<void()> cb);
    void ClearAnimEvents(const std::string& animName);

    // 装備中の武器管理: 最小実装 (WeaponType を Game/WeaponTypes.h で定義)
    // EquipWeapon は旧装備を返す。EquipWeapon により同じ武器の再装備は行わない。
    Game::WeaponType GetEquippedWeapon() const;
    // `newWeapon` を装備しようとする。装備が変更された場合は true を返し、
    // 以前の装備を `oldOut` に書き込む（未装備の場合は `Game::WeaponType::None`）。
    // 同一の武器を装備しようとした場合は何もせず false を返し、`oldOut` は変更しない。
    // これにより戻り値の曖昧さを排し、不要な再ドロップを防ぐ。
    bool TryEquipWeapon(Game::WeaponType newWeapon, Game::WeaponType* oldOut);

    // 後方互換のヘルパー。実際に交換が発生した場合は以前の武器を返し、
    // 交換が行われなかった場合は `Game::WeaponType::None` を返す。
    Game::WeaponType EquipWeapon(Game::WeaponType newWeapon);

private:
    // 元のベース MV1 モデルハンドル（表示に使うモデル）。アニメーション用のモデルは別途保持し、ここにアタッチして再生する。
    int baseModelHandle_ = -1; // 元のモデル（ここを差し替えない）

    // 現在描画に使う MV1 モデルハンドル（新方式では baseModelHandle_ と同じ値になる）
    int modelHandle_ = -1; // アクティブなモデルハンドル
    std::unordered_map<std::string,int> animModelHandles_; // map animation name -> MV1 model handle
    // アニメ名 -> アニメモデル内のモーションインデックス（ランタイムで MV1GetAnimIndex を呼ばずに済むようにする）
    std::unordered_map<std::string,int> animModelAnimIndex_;

    float x_ = 0.0f, y_ = 0.0f, z_ = 0.0f;

    // 描画スケール
    float visualScale_ = 0.04f; // さらに縮小したサイズ

    // HP
    float hp = 100.0f;
    float maxHp = 100.0f;

    // モード
    Mode mode_ = Mode::Melee;
    unsigned int lastModeSwitchTimeMs_ = 0;

    // Attack/dodge timing (ms)
    unsigned int lastAttackTimeMs_ = 0;
    float baseAttackCooldownMs_ = 300.0f;
    float attackCooldownMs_ = 300.0f;

    unsigned int lastDodgeTimeMs_ = 0;
    unsigned int dodgeCooldownMs_ = 800;
    unsigned int dodgeDurationMs_ = 300; // 無敵時間（ミリ秒）

    // Just dodge detection
    unsigned int lastIncomingTimeMs_ = 0;
    unsigned int baseJustWindowMs_ = 200;
    unsigned int justWindowMs_ = 200;

    // state
    bool justExecuted_ = false;

    // Aux units (left/right)
    AuxUnit* auxLeft = nullptr;
    AuxUnit* auxRight = nullptr;

    // 補助装備用ゲージ / 覚醒
    float auxGauge = 100.0f; // 0..100 自然回復
    float baseAuxGaugeRegenRate = 5.0f; // 秒あたり
    float auxGaugeRegenRate = 5.0f; // 現在の回復速度
    bool  awakened = false; // 覚醒中は発射コストが不要

    // アニメーションの長さとイベント（アニメごとに秒数を設定可能）
    std::unordered_map<std::string, float> animDurations_; // seconds

    // Lower-body (original) animation state
    std::string currentAnim_;
    float animTime_ = 0.0f; // seconds
    bool animLoop_ = true;

    struct AnimEvent { float time = 0.0f; bool fired = false; std::function<void()> cb; };
    std::unordered_map<std::string, std::vector<AnimEvent>> animEvents_;
    float prevAnimTime_ = 0.0f;

    // カメラ参照（カメラ基準の移動/向き計算に使用）
    CameraRig* camera_ = nullptr;

    // 向き（ヨー）の平滑化状態: 目標/現在のヨー（ラジアン）と回転速度（ラジアン/秒）
    float currentYaw_ = 0.0f; // 現在適用されているヨー（ラジアン）
    float targetYaw_ = 0.0f;  // 入力から計算した目標ヨー
    float yawTurnSpeed_ = 8.0f; // プレイヤーが目標方向へ向く速度（ラジアン/秒）

    // currently attached animation attach index (returned by MV1AttachAnim), or -1 if none (lower-body)
    int attachedAnimAttachIndex_ = -1;
    // previously attached animation attach index (for blending)
    int prevAttachedAnimAttachIndex_ = -1;

    // DXLib attach-animation total time for current and previous attachments (lower-body)
    float attachedAnimTotalTime_ = 0.0f;
    float prevAttachedAnimTotalTime_ = 0.0f;

    // store previous anim name and its seconds-based playhead for event/length lookup (lower-body)
    std::string prevAnimName_;
    float prevAnimTimeSeconds_ = 0.0f;
    bool prevAnimLoop_ = true;

    // blending control (lower-body)
    float animBlendRate_ = 1.0f; // 0..1, goes to 1 when blend completes
    float animBlendSpeed_ = 0.12f; // per-frame increment

    // Upper-body animation state (new)
    std::string upperAnim_;
    float upperAnimTime_ = 0.0f;
    bool upperAnimLoop_ = true;

    int upperAttachedAnimAttachIndex_ = -1;
    int prevUpperAttachedAnimAttachIndex_ = -1;

    float upperAttachedAnimTotalTime_ = 0.0f;
    float prevUpperAttachedAnimTotalTime_ = 0.0f;

    std::string prevUpperAnim_;
    float prevUpperAnimTimeSeconds_ = 0.0f;
    bool prevUpperAnimLoop_ = true;

    float upperAnimBlendRate_ = 1.0f; // blending for upper layer

    // whether this object owns the base model handle (loaded directly instead of via AssetsMgr)
    bool ownsBaseModel_ = false;

    // --- 新規: ジャンプ対応の簡易垂直物理 ---
    float velY_ = 0.0f;           // 垂直速度（単位: 単位/秒）
    bool onGround_ = true;        // 地上にいるか
    float gravity_ = -20.0f;      // 重力加速度（単位/秒^2）
    float jumpVelocity_ = 8.0f;   // ジャンプ開始時の初速（単位/秒）

    // timing helper for jump input (debounce)
    unsigned int lastJumpTimeMs_ = 0;

    // Track previous frame's attack button composite state to detect press edges (one attack per press)
    bool prevAttackBtnDown_ = false;
    // --- 装備（武器）関連の責務とメンバについて ---
    // 装備関連の状態は以下のメンバで管理しています。責務を明確にするためにコメントで説明します。
    //
    // 1) equippedWeapon_
    //    - 型: Game::WeaponType
    //    - 内容: 現在プレイヤーが装備している武器の種類。
    //    - 初期化: Player のメンバ初期値で None に初期化される。
    //    - 更新: Player::TryEquipWeapon / Player::EquipWeapon によって変更される。
    //    - 描画連携: 描画時に GetWeaponSpec(equippedWeapon_) を参照してモデルや補正を取得する。
    //
    // 2) equippedWeaponModelHandle_
    //    - 型: int (MV1 model handle)
    //    - 内容: 装備用モデルの MV1 ハンドル。AssetsMgr から取得したハンドルをキャッシュする。
    //    - 初期化: -1（無効）で初期化。
    //    - 更新: 装備切り替え時（TryEquipWeapon）に AssetsMgr から取得し、ロード失敗時は -1 のままにする。
    //    - 描画連携: Player::Draw 内でこのハンドルを使って MV1SetMatrix/MV1DrawModel を呼ぶ。
    //
    // 3) rightHandFrameIndex_
    //    - 型: int
    //    - 内容: base model 内での右手フレームのインデックスをキャッシュ。見つからない場合は -1。
    //    - 初期化: -1（未検出）で初期化。
    //    - 更新: ベースモデルロード/差し替え時にモデル内フレーム名リストを走査して決定する（Player 初期化やモデル変更処理）。
    //    - 描画連携: フレームが有効な場合は MV1GetFrameLocalWorldMatrix でフレームのワールド行列を取得して武器行列と合成する。
    //
    // 4) assets_
    //    - 型: AssetsMgr*
    //    - 内容: アセット管理器への参照。モデルやテクスチャの取得に使用する。
    //    - 初期化: コンストラクタの引数で渡されるか nullptr。
    //    - 更新: 基本的に所有権は持たず参照を保持する。外部で管理されるアセットマネージャを参照する。
    //
    // 注意点:
    //  - 装備状態の責務はここ（Player）で行っていますが、将来的には装備専用コンポーネントに切り出すことを検討してください。
    //  - 装備切り替え時は equippedWeapon_ と equippedWeaponModelHandle_ を整合させ、None のときはハンドルを -1 にすること。
    //  - 描画側はハンドルが -1 の場合は安全にスキップする実装としてください。

    // 将来の整理案: 装備状態をまとめる小さな構造体にできるようにテンプレを用意します。
    struct EquipmentState {
        Game::WeaponType equippedWeapon = Game::WeaponType::None; // 現在の武器種別
        int equippedModelHandle = -1; // 装備表示に使う MV1 ハンドル (-1 = none)
        int rightHandFrameIndex = -1; // 右手フレームのキャッシュ
        // AssetsMgr は Player 全体で使うためここには参照しない（必要ならポインタを追加）
    };

    // 現状は既存の個別メンバを維持して互換性を保つ。
    Game::WeaponType equippedWeapon_ = Game::WeaponType::None;
    // アセットマネージャ参照（モデルの取得に利用）
    AssetsMgr* assets_ = nullptr;

    // 右手フレーム番号のキャッシュ (-1 = 未検出)
    int rightHandFrameIndex_ = -1;

    // 装備中の武器モデルハンドル（AssetsMgr のキャッシュ先を参照する）
    int equippedWeaponModelHandle_ = -1;
};
