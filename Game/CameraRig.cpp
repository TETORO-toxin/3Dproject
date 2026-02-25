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
}

void CameraRig::Update(const VECTOR& playerPos, const VECTOR& targetPos, bool lockedOn)
{
    SetCameraNearFar(0.1f, 1200.0f);

    VECTOR desired;

    // time step for controller integration and spring
    float dt = 1.0f / 60.0f;

    // Poll inputs
    PadState pad = PollPad(0);
    InputState in = PollInput(0);

    // Determine controller presence (same heuristic as PollInput)
    bool controllerPresent = (pad.pad != 0) || (pad.xi.ThumbLX != 0) || (pad.xi.ThumbLY != 0) || (pad.xi.ThumbRX != 0) || (pad.xi.ThumbRY != 0) || (pad.xi.LeftTrigger != 0) || (pad.xi.RightTrigger != 0);

    if (lockedOn) {
        // locked on: keep previous behavior (behind player, look at target)
        VECTOR forward = VSub(targetPos, playerPos);
        float len = VSize(forward);
        if (len > 0.0001f) forward = VScale(forward, 1.0f / len);
        else forward = VGet(0.0f, 0.0f, 1.0f);

        desired = VAdd(playerPos, VScale(forward, -12.0f));
        desired.y += 6.0f;
        target_ = targetPos;
        frozenAbove_ = false; // allow movement when returning to normal
    } else {
        // If camera is frozen above player, skip input-driven orbiting to prevent jitter
        if (frozenAbove_) {
            // Keep desired at current camPos projected relative to player so spring doesn't fight the freeze
            desired = camPos_;
        } else {
            // Orbit controls: update yaw/pitch from controller right stick or mouse movement
            // Controller: use right stick axes (RX, RY). Mouse: use delta movement in pixels.
            if (controllerPresent) {
                // right stick gives -1..1 per axis; integrate over dt
                yaw_ += pad.RX * controllerYawSpeed_ * dt;
                pitch_ += -pad.RY * controllerPitchSpeed_ * dt; // invert Y
            } else {
                int mx = 0, my = 0;
                GetMousePoint(&mx, &my);
                if (prevMouseX_ == -1 && prevMouseY_ == -1) {
                    prevMouseX_ = mx; prevMouseY_ = my;
                }
                int dx = mx - prevMouseX_;
                int dy = my - prevMouseY_;
                prevMouseX_ = mx; prevMouseY_ = my;

                yaw_ += -dx * mouseSensitivity_; // negative so moving mouse right rotates camera right
                pitch_ += -dy * mouseSensitivity_;
            }

            // clamp pitch to avoid flipping
            const float maxPitch = 1.4f; // ~80 degrees
            const float minPitch = -1.2f; // allow looking slightly up
            if (pitch_ > maxPitch) pitch_ = maxPitch;
            if (pitch_ < minPitch) pitch_ = minPitch;

            // compute offset from spherical coords
            float cp = cosf(pitch_);
            float sp = sinf(pitch_);
            float cy = cosf(yaw_);
            float sy = sinf(yaw_);

            VECTOR offset = VGet(orbitRadius_ * cp * sy,
                                 orbitRadius_ * sp,
                                 orbitRadius_ * cp * cy);

            // desired is target (player) plus offset; raise target a bit so camera looks at upper body
            desired = VAdd(playerPos, VGet(offset.x, offset.y + 3.0f, offset.z));
            target_ = VAdd(playerPos, VGet(0.0f, 3.0f, 0.0f));
        }
    }

    // Prevent camera from approaching player if the desired position would place the camera
    // below the player's minimum signed height above the ground. In that case stop reducing
    // the camera's distance to the player (keep current radial distance) to avoid digging into ground.
    {
        // compute signed distances to plane
        VECTOR pToPlane = VSub(playerPos, groundPoint_);
        float playerSigned = Dot(pToPlane, groundNormal_);
        VECTOR dToPlane = VSub(desired, groundPoint_);
        float desiredSigned = Dot(dToPlane, groundNormal_);
        const float minAbovePlayer = 1.8f; // keep camera at least this above player's origin when centered

        if (desiredSigned < playerSigned + minAbovePlayer) {
            // if desired would be too low, and it would reduce radial distance, prevent approaching
            VECTOR curDir = VSub(camPos_, playerPos);
            VECTOR wantDir = VSub(desired, playerPos);
            float curDist = VSize(curDir);
            float wantDist = VSize(wantDir);
            if (wantDist < curDist && curDist > 0.0001f && wantDist > 0.0001f) {
                // keep desired at current radial distance from player along the desired direction
                VECTOR wantDirN = VScale(wantDir, 1.0f / wantDist);
                desired = VAdd(playerPos, VScale(wantDirN, curDist));
                // also nudge desired up along normal so it's at least the minimum signed height
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

    // simple spring integration: F = -k(x - x_desired) - c v
    VECTOR displacement = VSub(camPos_, desired);
    VECTOR springF = VScale(displacement, -springStiffness_);
    VECTOR dampingF = VScale(camVel_, -springDamping_);
    VECTOR acc = VAdd(springF, dampingF);

    // integrate velocity and position
    camVel_ = VAdd(camVel_, VScale(acc, dt));
    camPos_ = VAdd(camPos_, VScale(camVel_, dt));

    // Ground collision/clamping using plane: project camera position onto plane normal and ensure camera's distance from plane is >= minAboveGround.
    // Use a larger collision margin so the camera doesn't penetrate the ground.
    // This is the minimum signed distance from the ground plane the camera should maintain.
    const float minAboveGround = 1.0f; // meters above ground to keep (increased from 0.5f)
    // compute signed distance from plane: d = dot(camPos - groundPoint, groundNormal)
    VECTOR camToPlane = VSub(camPos_, groundPoint_);
    float signedDist = Dot(camToPlane, groundNormal_);
    if (signedDist < minAboveGround) {
        // desired signed distance
        // raise camera smoothly to at least minAboveGround
        float desiredDist = minAboveGround;
        float diff = desiredDist - signedDist;
        // move camera along normal by diff smoothly
        float lerp = 1.0f - powf(0.001f, dt);
        camPos_.x += groundNormal_.x * diff * lerp;
        camPos_.y += groundNormal_.y * diff * lerp;
        camPos_.z += groundNormal_.z * diff * lerp;

        // Smoothly move XZ toward player's XZ projected onto plane (move along plane tangent)
        // compute player's nearest point on plane
        VECTOR playerToPlane = VSub(playerPos, groundPoint_);
        float playerDist = Dot(playerToPlane, groundNormal_);
        VECTOR playerOnPlane = VSub(playerPos, VScale(groundNormal_, playerDist));

        // Immediately correct vertical penetration so camera never goes below the ground plane.
        float camSignedNow = Dot(VSub(camPos_, groundPoint_), groundNormal_);
        if (camSignedNow < minAboveGround) {
            float pushUp = minAboveGround - camSignedNow;
            camPos_ = VAdd(camPos_, VScale(groundNormal_, pushUp));
            camSignedNow = minAboveGround;
            // also remove any downward velocity along the normal to avoid re-penetration
            float velN = Dot(camVel_, groundNormal_);
            if (velN < 0.0f) {
                // don't kill downward velocity instantly; damp it to avoid a sudden jerk
                camVel_ = VSub(camVel_, VScale(groundNormal_, velN * 0.6f));
            }
        }

        // Project current camera position onto plane
        VECTOR camOnPlane = VSub(camPos_, VScale(groundNormal_, camSignedNow));

        // Direction along plane from camera to player
        VECTOR planeDir = VSub(playerOnPlane, camOnPlane);
        float planeDist = sqrtf(planeDir.x*planeDir.x + planeDir.y*planeDir.y + planeDir.z*planeDir.z);

        if (planeDist > 0.001f) {
            VECTOR planeDirN = VScale(planeDir, 1.0f / planeDist);

            // Desired speed proportional to distance (soft attractor)
            const float maxSlideSpeed = 1.6f;
            float desiredSpeed = planeDist * 0.5f;
            if (desiredSpeed < 0.02f) desiredSpeed = 0.02f;
            if (desiredSpeed > maxSlideSpeed) desiredSpeed = maxSlideSpeed;

            VECTOR desiredVelPlane = VScale(planeDirN, desiredSpeed);

            // velocity component along plane (remove normal component)
            float velAlongNormal = Dot(camVel_, groundNormal_);
            VECTOR camVelPlane = VSub(camVel_, VScale(groundNormal_, velAlongNormal));

            // Lerp current plane velocity toward desired to create a smooth ramp-up (avoids instant jumps)
            const float rampAlpha = 0.12f; // small -> smooth start
            camVelPlane.x = camVelPlane.x * (1.0f - rampAlpha) + desiredVelPlane.x * rampAlpha;
            camVelPlane.y = camVelPlane.y * (1.0f - rampAlpha) + desiredVelPlane.y * rampAlpha;
            camVelPlane.z = camVelPlane.z * (1.0f - rampAlpha) + desiredVelPlane.z * rampAlpha;

            // clamp plane speed just in case
            float spd = sqrtf(camVelPlane.x*camVelPlane.x + camVelPlane.y*camVelPlane.y + camVelPlane.z*camVelPlane.z);
            if (spd > maxSlideSpeed) camVelPlane = VScale(camVelPlane, maxSlideSpeed / spd);

            // reconstruct full velocity keeping normal component
            camVel_ = VAdd(VScale(groundNormal_, velAlongNormal), camVelPlane);

            // advance along-plane position using the new plane velocity (small dt step)
            camPos_ = VAdd(camPos_, VScale(camVelPlane, dt));

            // ensure we didn't sink below ground due to numerical issues
            camSignedNow = Dot(VSub(camPos_, groundPoint_), groundNormal_);
            if (camSignedNow < minAboveGround) {
                camPos_ = VAdd(camPos_, VScale(groundNormal_, (minAboveGround - camSignedNow)));
                // damp any remaining downward normal velocity
                float vn = Dot(camVel_, groundNormal_);
                if (vn < 0.0f) camVel_ = VSub(camVel_, VScale(groundNormal_, vn * 0.5f));
            }
        } else {
            // very near above player: remove plane velocity gently and snap XZ
            float velAlongNormal = Dot(camVel_, groundNormal_);
            VECTOR camVelPlane = VSub(camVel_, VScale(groundNormal_, velAlongNormal));
            camVelPlane = VScale(camVelPlane, 0.2f);
            camVel_ = VAdd(VScale(groundNormal_, velAlongNormal), camVelPlane);
            camPos_.x = playerOnPlane.x;
            camPos_.z = playerOnPlane.z;
        }

        // Damp velocities smoothly rather than zeroing to avoid abrupt stops
        camVel_.x *= 0.6f;
        camVel_.y *= 0.25f;
        camVel_.z *= 0.6f;
        // Slide camera along plane toward player to avoid digging into ground
        // (previous position-based slide replaced by velocity-based sliding)

        // recompute camera's projection and distance to player's plane-point for snap test
        camSignedNow = Dot(VSub(camPos_, groundPoint_), groundNormal_);
        camOnPlane = VSub(camPos_, VScale(groundNormal_, camSignedNow));
        VECTOR diffPlane = VSub(camOnPlane, playerOnPlane);
        float diffPlaneDist = sqrtf(diffPlane.x*diffPlane.x + diffPlane.y*diffPlane.y + diffPlane.z*diffPlane.z);

        const float snapThreshold = 0.3f;
        const float minAbovePlayer = 1.8f; // keep camera at least this above player's origin when centered

        if (diffPlaneDist < snapThreshold) {
            // place camera directly above player at signed height (playerDist + minAbovePlayer)
            float targetSigned = playerDist + minAbovePlayer;
            camPos_ = VAdd(playerOnPlane, VScale(groundNormal_, targetSigned));

            // ensure at least minAboveGround
            VECTOR camToPlane3 = VSub(camPos_, groundPoint_);
            float finalSigned = Dot(camToPlane3, groundNormal_);
            if (finalSigned < minAboveGround) {
                camPos_ = VAdd(camPos_, VScale(groundNormal_, minAboveGround - finalSigned));
            }

            // reduce velocities to avoid popping
            camVel_.x *= 0.05f;
            camVel_.y *= 0.01f;
            camVel_.z *= 0.05f;

            // look straight down at the player
            target_ = playerPos;

            // freeze camera to prevent further movement/jitter while looking straight down
            frozenAbove_ = true;
        } else {
            // Smoothly damp velocities while sliding
            camVel_.x *= 0.8f;
            camVel_.y *= 0.5f;
            camVel_.z *= 0.8f;

            // When sliding, ensure camera looks toward player's upper body so it's aligned
            target_ = VAdd(playerPos, VGet(0.0f, 2.0f, 0.0f));

            // if previously frozen but moved away beyond threshold, unfreeze
            frozenAbove_ = false;
        }
    }

    // Additional safeguard: if camera is very close in plane-projected XZ to player, ensure it's kept above player's head to avoid clipping into ground/player
    // compute vector in plane tangent from player to camera
    VECTOR playerToCam = VSub(camPos_, playerPos);
    // remove normal component
    float pn = Dot(playerToCam, groundNormal_);
    VECTOR tangent = VSub(playerToCam, VScale(groundNormal_, pn));
    float distXZ = sqrtf(tangent.x*tangent.x + tangent.z*tangent.z + tangent.y*tangent.y); // distance along plane
    const float closeThreshold = 0.7f; // meters
    const float minAbovePlayer = 1.8f; // keep camera at least this above player's origin when centered (signed distance)
    // compute player's signed distance to plane
    VECTOR pToPlane = VSub(playerPos, groundPoint_);
    float playerSigned = Dot(pToPlane, groundNormal_);
    if (distXZ < closeThreshold) {
        // ensure camera has enough signed height over player
        if ( (signedDist) < (playerSigned + minAbovePlayer) ) {
            float targetSigned = playerSigned + minAbovePlayer;
            float push = targetSigned - signedDist;
            float pushLerp = 1.0f - powf(0.001f, dt);
            camPos_.x += groundNormal_.x * push * pushLerp;
            camPos_.y += groundNormal_.y * push * pushLerp;
            camPos_.z += groundNormal_.z * push * pushLerp;
            camVel_.y *= 0.2f;

            // also nudge along tangent outward
            float nudge = (closeThreshold - distXZ) * 0.5f;
            if (distXZ > 0.0001f) {
                camPos_.x += (tangent.x / distXZ) * -nudge;
                camPos_.y += (tangent.y / distXZ) * -nudge;
                camPos_.z += (tangent.z / distXZ) * -nudge;
            } else {
                camPos_.x += 0.01f; camPos_.y += 0.01f; camPos_.z += 0.01f;
            }
            // look at player's upper body
            target_ = VAdd(playerPos, VGet(0.0f, 2.2f, 0.0f));
        }
    }

    // If frozen above, ensure camera position/velocity remain stable (prevent spring from moving it)
    if (frozenAbove_) {
        // zero small velocities and lock position to prevent jitter
        if (VSize(camVel_) < 0.01f) {
            camVel_.x = camVel_.y = camVel_.z = 0.0f;
        } else {
            // damp strongly
            camVel_.x *= 0.05f;
            camVel_.y *= 0.02f;
            camVel_.z *= 0.05f;
        }
        // lock camPos_ to current to avoid drift
        // (no changes; we've already set camPos_ earlier when snapping)
    }

    SetCameraPositionAndTarget_UpVecY(camPos_, target_);
}

// Return camera forward vector projected to XZ plane and normalized.
VECTOR CameraRig::GetForwardXZ() const
{
    VECTOR f = VSub(target_, camPos_);
    f.y = 0.0f;
    float len = sqrtf(f.x*f.x + f.z*f.z);
    if (len < 0.0001f) return VGet(0.0f, 0.0f, 1.0f);
    return VGet(f.x / len, 0.0f, f.z / len);
}

// getters
VECTOR CameraRig::GetCameraPosition() const { return camPos_; }
VECTOR CameraRig::GetForward() const { return VSub(target_, camPos_); }
