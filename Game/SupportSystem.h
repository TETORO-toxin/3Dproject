#pragma once
#include "Support.h"

class Player;
class Enemy;

class SupportSystem
{
public:
    SupportSystem();

    // Tick each frame with dt seconds
    void Tick(float dt, Player* player);

    // Activation APIs (return true if successfully activated)
    bool ActivateAwakening(Player* player);
    bool EmergencyHeal(Player* player, float amount);
    bool RemoteSupportAttack(Enemy* target, float weakGain);

    // Query cooldowns
    float GetHealCooldownRemaining() const { return healCooldownRemaining_; }
    float GetRemoteCooldownRemaining() const { return remoteCooldownRemaining_; }
    const SupportStatus& GetSupportStatus() const { return support_; }

private:
    SupportStatus support_;
    float healCooldownMax_ = 20.0f; // seconds
    float remoteCooldownMax_ = 8.0f; // seconds
    float healCooldownRemaining_ = 0.0f;
    float remoteCooldownRemaining_ = 0.0f;
};
