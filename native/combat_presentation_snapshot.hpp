#pragma once

#include "Game.hpp"
#include "HumanVisual.hpp"

// Presentation contracts expose the native mechanic phases and resolved poses
// together. Godot may interpolate these values, but must never use animation
// playback position to decide damage, displacement, grabs, or recovery.
struct PlayerActionPresentationSnapshot {
    unsigned short sequence = 0;
    int comboIndex = 0;
    int variant = 0;
    float visualTimer = 0.0f;
    float visualDuration = 0.0f;
    float normalizedPhase = 0.0f;
    float range = 0.0f;
    float hitRadius = 0.0f;
    float damage = 0.0f;
    Vec3 origin;
    Vec3 direction;
    Vec3 impact;
    bool visualHit = false;
    unsigned int hitMask = 0;
    bool locomotionLunge = false;
    bool airLungePending = false;
    bool airLungeLandingPending = false;
    float airLungeTimer = 0.0f;
    float airLungeRotation = 0.0f;
    float landingRecovery = 0.0f;
    float landingRecoveryDuration = 0.0f;
    float landingPosePitch = 0.0f;
    float wallGripTimer = 0.0f;
    Vec3 playerPosition;
    Vec3 playerVelocity;

    static PlayerActionPresentationSnapshot from(const GameState& state) {
        PlayerActionPresentationSnapshot snapshot;
        const auto& action = state.meleeVisual;
        snapshot.sequence = action.actionSequence;
        snapshot.comboIndex = action.comboIndex;
        snapshot.variant = action.variant;
        snapshot.visualTimer = action.visualTimer;
        snapshot.visualDuration = action.visualDuration;
        snapshot.normalizedPhase = action.visualDuration > 0.0001f
            ? clampf(1.0f - action.visualTimer / action.visualDuration, 0.0f, 1.0f) : 1.0f;
        snapshot.range = action.range;
        snapshot.hitRadius = action.hitRadius;
        snapshot.damage = action.damage;
        snapshot.origin = action.origin;
        snapshot.direction = action.direction;
        snapshot.impact = action.impact;
        snapshot.visualHit = action.visualHit;
        snapshot.hitMask = action.hitMask;
        snapshot.locomotionLunge = action.locomotionLunge;
        snapshot.airLungePending = action.airLungePending;
        snapshot.airLungeLandingPending = action.airLungeLandingPending;
        snapshot.airLungeTimer = action.airLungeTimer;
        snapshot.airLungeRotation = action.airLungeRotation;
        snapshot.landingRecovery = action.landingRecovery;
        snapshot.landingRecoveryDuration = action.landingRecoveryDuration;
        snapshot.landingPosePitch = action.landingPosePitch;
        snapshot.wallGripTimer = action.wallGripTimer;
        snapshot.playerPosition = state.player.pos;
        snapshot.playerVelocity = state.player.vel;
        return snapshot;
    }
};

struct TargetPresentationSnapshot {
    int index = -1;
    Vec3 position;
    Vec3 velocity;
    bool alive = false;
    bool slurpable = false;
    bool brute = false;
    float scale = 1.0f;
    float armor = 0.0f;
    float health = 0.0f;
    float visualYaw = 0.0f;
    float locomotionAmount = 0.0f;
    float locomotionPhase = 0.0f;
    float animationTime = 0.0f;
    float hitAmount = 0.0f;
    float hitDirectionLocal = 0.0f;
    bool attackActive = false;
    float attackTimer = 0.0f;
    float attackPhase = 0.0f;
    float attackCommitPhase = HUMAN_SWING_COMMIT_PHASE;
    float attackEndPhase = HUMAN_SWING_END_PHASE;
    int attackVariant = 0;
    bool attackHit = false;
    Vec3 attackDirection;
    int attackTargetPlayerId = 0;
    int grabbedPlayerId = -1;
    float grabCooldown = 0.0f;
    float bodyPitch = 0.0f;
    float bodyRoll = 0.0f;
    float leftFootContact = 0.0f;
    float rightFootContact = 0.0f;
    Vec3 leftFootTarget;
    Vec3 rightFootTarget;
    bool tetherVisible = false;
    Vec3 tetherAnchor;
    Vec3 tetherDestination;
    float tetherWidth = 0.0f;
    bool bodyFallen = false;
    bool bodyAuthoritative = false;
    HumanVisualPose pose;
    HumanReactionVisual reaction;
    Vec3 headCenter;
    float headRadius = 0.0f;
    Vec3 torsoCenter;
    Vec3 torsoSize;

    static TargetPresentationSnapshot from(const GameState& state, int index) {
        TargetPresentationSnapshot snapshot;
        if (index < 0 || index >= TARGET_COUNT) return snapshot;
        const auto& target = state.targets[index];
        const auto body = target.humanBodyPresentation();
        snapshot.index = index;
        snapshot.position = target.pos;
        snapshot.velocity = target.vel;
        snapshot.alive = target.alive;
        snapshot.slurpable = target.slurpable;
        snapshot.brute = target.brute;
        snapshot.scale = target.scale;
        snapshot.armor = target.armor;
        snapshot.health = target.health;
        snapshot.visualYaw = target.visualYaw;
        snapshot.locomotionAmount = target.locomotionAmount;
        snapshot.locomotionPhase = target.visualWalkPhase;
        snapshot.animationTime = target.humanAnimationTime;
        snapshot.hitAmount = target.hitFlash;
        snapshot.hitDirectionLocal = target.hitDirectionLocal;
        snapshot.attackActive = target.attackTimer > 0.0f;
        snapshot.attackTimer = target.attackTimer;
        snapshot.attackPhase = snapshot.attackActive
            ? clampf(1.0f - target.attackTimer / HUMAN_SWING_ATTACK_DURATION, 0.0f, 1.0f) : 0.0f;
        snapshot.attackVariant = target.attackVariant;
        snapshot.attackHit = target.attackHit;
        snapshot.attackDirection = target.attackDirection;
        snapshot.attackTargetPlayerId = target.attackTargetPlayerId;
        snapshot.grabbedPlayerId = target.grabbedPlayerId;
        snapshot.grabCooldown = target.grabCooldown;
        snapshot.bodyPitch = body.pitch();
        snapshot.bodyRoll = body.roll();
        snapshot.leftFootContact = body.leftFootContact();
        snapshot.rightFootContact = body.rightFootContact();
        snapshot.leftFootTarget = body.leftFootTarget();
        snapshot.rightFootTarget = body.rightFootTarget();
        snapshot.tetherVisible = target.tetherVisible;
        snapshot.tetherAnchor = target.tetherAnchor;
        snapshot.tetherDestination = target.tetherDestination;
        snapshot.tetherWidth = target.tetherWidth;
        snapshot.bodyFallen = body.fallen();
        snapshot.bodyAuthoritative = body.authoritative();
        snapshot.reaction = target.visualReaction;
        snapshot.pose = makeHumanVisualPose(target.visualYaw, target.scale,
            state.time, target.visualReaction, target.alive && !target.slurpable, body);
        snapshot.headCenter = {target.pos.x,
            (HUMAN_VISUAL_SPEC.totalHeight - HUMAN_VISUAL_SPEC.headRadius) * target.scale,
            target.pos.z};
        snapshot.headRadius = HUMAN_VISUAL_SPEC.headRadius * target.scale;
        snapshot.torsoCenter = target.pos + Vec3{0, HUMAN_VISUAL_SPEC.centerOfMassHeight * target.scale, 0};
        snapshot.torsoSize = {HUMAN_VISUAL_SPEC.torsoWidth * target.scale,
            HUMAN_VISUAL_SPEC.torsoHeight * target.scale,
            HUMAN_VISUAL_SPEC.torsoDepth * target.scale};
        return snapshot;
    }
};
