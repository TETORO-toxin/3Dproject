#include "UI3D2D.h"
#include "Enemy.h"

void DrawEnemyUI(const Enemy& e, bool lockedOn)
{
    VECTOR head = VAdd(e.GetPosition(), VGet(0.0f, 2.0f, 0.0f));
    VECTOR s = ConvWorldPosToScreenPos(head);

    // background
    DrawBox((int)s.x-30, (int)s.y-40, (int)s.x+30, (int)s.y-36, GetColor(40,40,40), TRUE);
    int w = (int)(60.0f * (e.GetWeakGauge()/100.0f));
    DrawBox((int)s.x-30, (int)s.y-40, (int)s.x-30 + w, (int)s.y-36, GetColor(0,180,255), TRUE);

    if (lockedOn) {
        DrawCircle((int)s.x, (int)s.y-60, 12, GetColor(255,0,0), FALSE, 2);
    }
}

void DrawLockonReticleAtScreen(const VECTOR& worldPos)
{
    VECTOR s = ConvWorldPosToScreenPos(worldPos);
    DrawCircle((int)s.x, (int)s.y, 18, GetColor(0,255,255), FALSE, 2);
}

void DrawIncomingWarning(const VECTOR& worldPos, const char* msg)
{
    VECTOR s = ConvWorldPosToScreenPos(worldPos);
    DrawFormatString((int)s.x, (int)s.y-80, GetColor(255,160,0), "%s", msg);
}
