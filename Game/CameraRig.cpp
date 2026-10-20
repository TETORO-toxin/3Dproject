#include "CameraRig.h"
#include "../Sys/Input.h"
#include <cmath>

static float Dot(const VECTOR& a, const VECTOR& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static VECTOR Normalize(const VECTOR& v) {
    float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (len < 0.000001f) return VGet(0,0,1);
    return VGet(v.x/len, v.y/len, v.z/len);
}

CameraRig::CameraRig()
{
    lastForwardXZ_ = VGet(0.0f, 0.0f, 1.0f);
}

void CameraRig::SetTarget(float x, float y, float z)
{
    target_.x = x; target_.y = y; target_.z = z;
}

void CameraRig::SetGroundPlane(const VECTOR& point, const VECTOR& normal)
{
    groundPoint_ = point;
    groundNormal_ = Normalize(normal);
}

void CameraRig::Update()
{
    SetCameraNearFar(0.1f, 1200.0f);
    SetCameraPositionAndTarget_UpVecY(camPos_, target_);
    VECTOR f = VSub(target_, camPos_);
    f.y = 0.0f;
    float len = sqrtf(f.x*f.x + f.z*f.z);
    if (len > 0.0001f) {
        lastForwardXZ_ = VGet(f.x / len, 0.0f, f.z / len);
    }
}

void CameraRig::Update(const VECTOR& playerPos, const VECTOR& targetPos, bool lockedOn)
{
    // legacy: assume 60fps
    Update(playerPos, targetPos, lockedOn, 1.0f/60.0f);
}

void CameraRig::Update(const VECTOR& playerPos, const VECTOR& targetPos, bool lockedOn, float dt)
{
    // ���̍X�V�֐��͖��t���[���Ăяo�����A�J�����̈ʒu�E�������v�Z���܂��B
    // playerPos: �v���C���[�̃��[���h���W
    // targetPos: ���b�N�I���ΏۂȂǂ̃^�[�Q�b�g���W
    // lockedOn: ���b�N�I�������ǂ����̃t���O

    SetCameraNearFar(0.1f, 1200.0f);

    VECTOR desired;

    // Poll inputs
    PadState pad = PollPad(0);
    InputState in = PollInput(0);

    // �R���g���[�����ڑ������Ă��邩���肷���iPollInput �Ɠ����q���[���X�e�B�b�N�j
    bool controllerPresent = (pad.pad != 0) || (pad.xi.ThumbLX != 0) || (pad.xi.ThumbLY != 0) || (pad.xi.ThumbRX != 0) || (pad.xi.ThumbRY != 0) || (pad.xi.LeftTrigger != 0) || (pad.xi.RightTrigger != 0);

    if (lockedOn) {
        // Specific shoulder-over follow mode for target tracking (coin lock, enemy lock):
        // keep the camera just behind the player's shoulder, offset to the side closest to the target,
        // while the actual look-at point stays on the tracked target.
        VECTOR playerToTarget = VSub(targetPos, playerPos);
        float dist = VSize(playerToTarget);
        if (dist > 0.0001f) playerToTarget = VScale(playerToTarget, 1.0f / dist);
        else playerToTarget = VGet(0.0f, 0.0f, 1.0f);

        VECTOR forwardXZ = VGet(playerToTarget.x, 0.0f, playerToTarget.z);
        float forwardLen = VSize(forwardXZ);
        if (forwardLen > 0.0001f) forwardXZ = VScale(forwardXZ, 1.0f / forwardLen);
        else forwardXZ = VGet(0.0f, 0.0f, 1.0f);

        VECTOR rightXZ = VGet(forwardXZ.z, 0.0f, -forwardXZ.x);
        float rightLen = VSize(rightXZ);
        if (rightLen > 0.0001f) rightXZ = VScale(rightXZ, 1.0f / rightLen);
        else rightXZ = VGet(1.0f, 0.0f, 0.0f);

        // Choose the shoulder closest to the target side.
        VECTOR playerToTargetXZ = VGet(targetPos.x - playerPos.x, 0.0f, targetPos.z - playerPos.z);
        float sideBias = Dot(playerToTargetXZ, rightXZ);
        VECTOR shoulderDir = VScale(rightXZ, sideBias >= 0.0f ? 1.0f : -1.0f);

        // Aim the camera from behind the player's shoulder so the coin is visible at the edge of frame.
        VECTOR camOffset = VAdd(VScale(playerToTarget, -8.0f), VAdd(VScale(shoulderDir, 1.5f), VGet(0.0f, 2.8f, 0.0f)));
        desired = VAdd(playerPos, camOffset);
        target_ = targetPos;
        frozenAbove_ = false;
    } else {
        // �ʏ펞: �I�[�r�b�g�������}�E�X/�X�e�B�b�N���͂ɂ������]���s��
        // frozenAbove_ �� true �̂Ƃ��́A�v���C���[�̐^���ɌŒ肳���Ă����̂ŉ��]�������X�L�b�v
        if (frozenAbove_) {
            // �X�v�����O���Œ����ԂƑ����Ȃ��悤�Aplayer �ɑ΂��錻�݂� camPos ���ێ�����
            desired = camPos_;
        } else {
            // �I�[�r�b�g����: �R���g���[���E�X�e�B�b�N�܂��̓}�E�X�ړ����� yaw/pitch ���X�V
            // �R���g���[��: �E�X�e�B�b�N�� (RX, RY)�B�}�E�X: ���ʒ��S�����̕΍����p��������
            if (controllerPresent) {
                // right stick gives -1..1 per axis; integrate over dt
                yaw_ += pad.RX * controllerYawSpeed_ * dt;
                pitch_ += -pad.RY * controllerPitchSpeed_ * dt; // invert Y
            } else {
                const int screenW = 800;
                const int screenH = 600;

                int mx = 0, my = 0;
                GetMousePoint(&mx, &my);

                int centerX = screenW / 2;
                int centerY = screenH / 2;

                if (prevMouseX_ == -1 && prevMouseY_ == -1) {
                    SetMousePoint(centerX, centerY);
                    prevMouseX_ = centerX;
                    prevMouseY_ = centerY;
                    mx = centerX; my = centerY;
                }

                int dx = mx - centerX;
                int dy = my - centerY;

                yaw_ += -dx * mouseSensitivity_;
                pitch_ += -dy * mouseSensitivity_;

                SetMousePoint(centerX, centerY);
                prevMouseX_ = centerX; prevMouseY_ = centerY;
            }

            const float maxPitch = 1.4f;
            const float minPitch = -1.2f;
            if (pitch_ > maxPitch) pitch_ = maxPitch;
            if (pitch_ < minPitch) pitch_ = minPitch;

            float cp = cosf(pitch_);
            float sp = sinf(pitch_);
            float cy = cosf(yaw_);
            float sy = sinf(yaw_);

            VECTOR offset = VGet(orbitRadius_ * cp * sy,
                                 orbitRadius_ * sp,
                                 orbitRadius_ * cp * cy);

            desired = VAdd(playerPos, VGet(offset.x, offset.y + 3.0f, offset.z));
            target_ = VAdd(playerPos, VGet(0.0f, 3.0f, 0.0f));
        }
    }

    {
        VECTOR pToPlane = VSub(playerPos, groundPoint_);
        float playerSigned = Dot(pToPlane, groundNormal_);
        VECTOR dToPlane = VSub(desired, groundPoint_);
        float desiredSigned = Dot(dToPlane, groundNormal_);
        const float minAbovePlayer = 1.8f;

        if (desiredSigned < playerSigned + minAbovePlayer) {
            VECTOR curDir = VSub(camPos_, playerPos);
            VECTOR wantDir = VSub(desired, playerPos);
            float curDist = VSize(curDir);
            float wantDist = VSize(wantDir);
            if (wantDist < curDist && curDist > 0.0001f && wantDist > 0.0001f) {
                VECTOR wantDirN = VScale(wantDir, 1.0f / wantDist);
                desired = VAdd(playerPos, VScale(wantDirN, curDist));
                float newDesiredSigned = Dot(VSub(desired, groundPoint_), groundNormal_);
                if (newDesiredSigned < playerSigned + minAbovePlayer) {
                    float push = (playerSigned + minAbovePlayer) - newDesiredSigned;
                    desired.x += groundNormal_.x * push;
                    desired.y += groundNormal_.y * push;
                    desired.z += groundNormal_.z * push;
                }
            }
        }
    }

    VECTOR displacement = VSub(camPos_, desired);
    VECTOR springF = VScale(displacement, -springStiffness_);
    VECTOR dampingF = VScale(camVel_, -springDamping_);
    VECTOR acc = VAdd(springF, dampingF);

    camVel_ = VAdd(camVel_, VScale(acc, dt));
    camPos_ = VAdd(camPos_, VScale(camVel_, dt));

    const float minAboveGround = 1.0f;
    VECTOR camToPlane = VSub(camPos_, groundPoint_);
    float signedDist = Dot(camToPlane, groundNormal_);
    if (signedDist < minAboveGround) {
        float desiredDist = minAboveGround;
        float diff = desiredDist - signedDist;
        float lerp = 1.0f - powf(0.001f, dt);
        camPos_.x += groundNormal_.x * diff * lerp;
        camPos_.y += groundNormal_.y * diff * lerp;
        camPos_.z += groundNormal_.z * diff * lerp;

        VECTOR playerToPlane = VSub(playerPos, groundPoint_);
        float playerDist = Dot(playerToPlane, groundNormal_);
        VECTOR playerOnPlane = VSub(playerPos, VScale(groundNormal_, playerDist));

        float camSignedNow = Dot(VSub(camPos_, groundPoint_), groundNormal_);
        if (camSignedNow < minAboveGround) {
            float pushUp = minAboveGround - camSignedNow;
            camPos_ = VAdd(camPos_, VScale(groundNormal_, pushUp));
            camSignedNow = minAboveGround;
            float velN = Dot(camVel_, groundNormal_);
            if (velN < 0.0f) {
                camVel_ = VSub(camVel_, VScale(groundNormal_, velN * 0.6f));
            }
        }

        VECTOR camOnPlane = VSub(camPos_, VScale(groundNormal_, camSignedNow));
        VECTOR planeDir = VSub(playerOnPlane, camOnPlane);
        float planeDist = sqrtf(planeDir.x*planeDir.x + planeDir.y*planeDir.y + planeDir.z*planeDir.z);

        if (planeDist > 0.001f) {
            VECTOR planeDirN = VScale(planeDir, 1.0f / planeDist);
            const float maxSlideSpeed = 1.6f;
            float desiredSpeed = planeDist * 0.5f;
            if (desiredSpeed < 0.02f) desiredSpeed = 0.02f;
            if (desiredSpeed > maxSlideSpeed) desiredSpeed = maxSlideSpeed;

            VECTOR desiredVelPlane = VScale(planeDirN, desiredSpeed);
            float velAlongNormal = Dot(camVel_, groundNormal_);
            VECTOR camVelPlane = VSub(camVel_, VScale(groundNormal_, velAlongNormal));

            const float rampAlpha = 0.12f;
            camVelPlane.x = camVelPlane.x * (1.0f - rampAlpha) + desiredVelPlane.x * rampAlpha;
            camVelPlane.y = camVelPlane.y * (1.0f - rampAlpha) + desiredVelPlane.y * rampAlpha;
            camVelPlane.z = camVelPlane.z * (1.0f - rampAlpha) + desiredVelPlane.z * rampAlpha;

            float spd = sqrtf(camVelPlane.x*camVelPlane.x + camVelPlane.y*camVelPlane.y + camVelPlane.z*camVelPlane.z);
            if (spd > maxSlideSpeed) camVelPlane = VScale(camVelPlane, maxSlideSpeed / spd);

            camVel_ = VAdd(VScale(groundNormal_, velAlongNormal), camVelPlane);
            camPos_ = VAdd(camPos_, VScale(camVelPlane, dt));

            camSignedNow = Dot(VSub(camPos_, groundPoint_), groundNormal_);
            if (camSignedNow < minAboveGround) {
                camPos_ = VAdd(camPos_, VScale(groundNormal_, (minAboveGround - camSignedNow)));
                float vn = Dot(camVel_, groundNormal_);
                if (vn < 0.0f) camVel_ = VSub(camVel_, VScale(groundNormal_, vn * 0.5f));
            }
        } else {
            float velAlongNormal = Dot(camVel_, groundNormal_);
            VECTOR camVelPlane = VSub(camVel_, VScale(groundNormal_, velAlongNormal));
            camVelPlane = VScale(camVelPlane, 0.2f);
            camVel_ = VAdd(VScale(groundNormal_, velAlongNormal), camVelPlane);
            camPos_.x = playerOnPlane.x;
            camPos_.z = playerOnPlane.z;
        }

        camVel_.x *= 0.6f;
        camVel_.y *= 0.25f;
        camVel_.z *= 0.6f;

        camSignedNow = Dot(VSub(camPos_, groundPoint_), groundNormal_);
        camOnPlane = VSub(camPos_, VScale(groundNormal_, camSignedNow));
        VECTOR diffPlane = VSub(camOnPlane, playerOnPlane);
        float diffPlaneDist = sqrtf(diffPlane.x*diffPlane.x + diffPlane.y*diffPlane.y + diffPlane.z*diffPlane.z);

        const float snapThreshold = 0.3f;
        const float minAbovePlayer = 1.8f;

        if (diffPlaneDist < snapThreshold) {
            float targetSigned = playerDist + minAbovePlayer;
            camPos_ = VAdd(playerOnPlane, VScale(groundNormal_, targetSigned));

            VECTOR camToPlane3 = VSub(camPos_, groundPoint_);
            float finalSigned = Dot(camToPlane3, groundNormal_);
            if (finalSigned < minAboveGround) {
                camPos_ = VAdd(camPos_, VScale(groundNormal_, minAboveGround - finalSigned));
            }

            camVel_.x *= 0.05f;
            camVel_.y *= 0.01f;
            camVel_.z *= 0.05f;

            target_ = playerPos;
            frozenAbove_ = true;
        } else {
            camVel_.x *= 0.8f;
            camVel_.y *= 0.5f;
            camVel_.z *= 0.8f;

            target_ = VAdd(playerPos, VGet(0.0f, 2.0f, 0.0f));
            frozenAbove_ = false;
        }
    }

    VECTOR playerToCam = VSub(camPos_, playerPos);
    float pn = Dot(playerToCam, groundNormal_);
    VECTOR tangent = VSub(playerToCam, VScale(groundNormal_, pn));
    float distXZ = sqrtf(tangent.x*tangent.x + tangent.z*tangent.z + tangent.y*tangent.y);
    const float closeThreshold = 0.7f;
    const float minAbovePlayer = 1.8f;
    VECTOR pToPlane = VSub(playerPos, groundPoint_);
    float playerSigned = Dot(pToPlane, groundNormal_);
    if (distXZ < closeThreshold) {
        if (signedDist < (playerSigned + minAbovePlayer)) {
            float targetSigned = playerSigned + minAbovePlayer;
            float push = targetSigned - signedDist;
            float pushLerp = 1.0f - powf(0.001f, dt);
            camPos_.x += groundNormal_.x * push * pushLerp;
            camPos_.y += groundNormal_.y * push * pushLerp;
            camPos_.z += groundNormal_.z * push * pushLerp;
            camVel_.y *= 0.2f;

            float nudge = (closeThreshold - distXZ) * 0.5f;
            if (distXZ > 0.0001f) {
                camPos_.x += (tangent.x / distXZ) * -nudge;
                camPos_.y += (tangent.y / distXZ) * -nudge;
                camPos_.z += (tangent.z / distXZ) * -nudge;
            } else {
                camPos_.x += 0.01f; camPos_.y += 0.01f; camPos_.z += 0.01f;
            }
            target_ = VAdd(playerPos, VGet(0.0f, 2.2f, 0.0f));
        }
    }

    if (frozenAbove_) {
        if (VSize(camVel_) < 0.01f) {
            camVel_.x = camVel_.y = camVel_.z = 0.0f;
        } else {
            camVel_.x *= 0.05f;
            camVel_.y *= 0.02f;
            camVel_.z *= 0.05f;
        }
    }

    SetCameraPositionAndTarget_UpVecY(camPos_, target_);

    VECTOR f = VSub(target_, camPos_);
    f.y = 0.0f;
    float len = sqrtf(f.x*f.x + f.z*f.z);
    if (len > 0.0001f) {
        lastForwardXZ_ = VGet(f.x / len, 0.0f, f.z / len);
    }
}

// Return camera forward vector projected to XZ plane and normalized.
VECTOR CameraRig::GetForwardXZ() const
{
    VECTOR f = VSub(target_, camPos_);
    f.y = 0.0f;
    float len = sqrtf(f.x*f.x + f.z*f.z);
    if (len < 0.0001f) {
        return lastForwardXZ_;
    }

    VECTOR forward = VGet(f.x / len, 0.0f, f.z / len);
    return forward;
}

// getters
VECTOR CameraRig::GetCameraPosition() const { return camPos_; }
VECTOR CameraRig::GetForward() const { return VSub(target_, camPos_); }
