#pragma once
#include "DxLib.h"

class Enemy;

// Draw UI elements projected from 3D world to 2D screen
void DrawEnemyUI(const Enemy& e, bool lockedOn = false);
void DrawLockonReticleAtScreen(const VECTOR& worldPos);
void DrawIncomingWarning(const VECTOR& worldPos, const char* msg);
