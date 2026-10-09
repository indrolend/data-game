#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "Math.hpp"

constexpr float HUMAN_SWING_ATTACK_DURATION = 0.86f;
constexpr float HUMAN_SWING_COMMIT_PHASE = 0.30f;
constexpr float HUMAN_SWING_END_PHASE = 0.72f;

struct HumanVisualSpec {
    float totalHeight;
    float shoulderWidth;
    float torsoHeight;
    float torsoWidth;
    float torsoDepth;
    float pelvisHeight;
    float pelvisWidth;
    float pelvisDepth;
    float headRadius;
    float neckHeight;
    float upperArmLength;
    float forearmLength;
    float thighLength;
    float shinLength;
    float footLength;
    float footHeight;
    float handSize;
    float rootGroundOffset;
    float centerOfMassHeight;
    float forwardYawOffset;
    float normalScale;
    float bruteScale;
};

constexpr HumanVisualSpec HUMAN_VISUAL_SPEC{
    1.16f,  // totalHeight after source-model normalization.
    0.34f,  // shoulderWidth: proportional reconstruction from normalized FBX height.
    0.38f,
    0.27f,
    0.15f,
    0.16f,
    0.24f,
    0.14f,
    0.105f,
    0.89f,
    0.24f,
    0.22f,
    0.33f,
    0.32f,
    0.19f,
    0.065f,
    0.07f,
    0.0f,
    0.56f,
    DB_PI,
    1.0f,
    1.7f
};

struct HumanVisualPose {
    float yaw = 0.0f;
    float scale = 1.0f;
    float morphShrink = 1.0f;
    float rootBob = 0.0f;
    float torsoPitch = 0.0f;
    float torsoRoll = 0.0f;
    float headPitch = 0.0f;
    float leftArmSwing = 0.0f;
    float rightArmSwing = 0.0f;
    float leftLegSwing = 0.0f;
    float rightLegSwing = 0.0f;
    float hitLean = 0.0f;
    float soulMorph = 0.0f;
    float vacuumLean = 0.0f;
    float collapse = 0.0f;
    Vec3 expressiveScale{1.0f, 1.0f, 1.0f};
};

struct HumanBodyPresentation {
    std::int16_t pitchQuantized = 0;
    std::int16_t rollQuantized = 0;
    std::uint8_t leftFootContactByte = 255;
    std::uint8_t rightFootContactByte = 255;
    std::uint8_t flags = 0;
    float pitch() const { return static_cast<float>(pitchQuantized)*(1.5f/32767.0f); }
    float roll() const { return static_cast<float>(rollQuantized)*(1.5f/32767.0f); }
    float leftFootContact() const { return static_cast<float>(leftFootContactByte)/255.0f; }
    float rightFootContact() const { return static_cast<float>(rightFootContactByte)/255.0f; }
    bool fallen() const { return (flags&1u)!=0; }
    bool authoritative() const { return (flags&2u)!=0; }
};

inline HumanBodyPresentation makeHumanBodyPresentation(float pitch,float roll,float leftContact,float rightContact,bool fallen,bool authoritative) {
    HumanBodyPresentation result;
    result.pitchQuantized=static_cast<std::int16_t>(clampf(pitch,-1.5f,1.5f)*(32767.0f/1.5f));
    result.rollQuantized=static_cast<std::int16_t>(clampf(roll,-1.5f,1.5f)*(32767.0f/1.5f));
    result.leftFootContactByte=static_cast<std::uint8_t>(clampf(leftContact,0.0f,1.0f)*255.0f+0.5f);
    result.rightFootContactByte=static_cast<std::uint8_t>(clampf(rightContact,0.0f,1.0f)*255.0f+0.5f);
    result.flags=static_cast<std::uint8_t>((fallen?1u:0u)|(authoritative?2u:0u));
    return result;
}

inline HumanBodyPresentation interpolateHumanBodyPresentation(
    const HumanBodyPresentation& previous,const HumanBodyPresentation& current,float alpha) {
    if(!previous.authoritative()||!current.authoritative())return current;
    alpha=clampf(alpha,0.0f,1.0f);
    HumanBodyPresentation result=current;
    const float pitch=previous.pitch()+(current.pitch()-previous.pitch())*alpha;
    const float roll=previous.roll()+(current.roll()-previous.roll())*alpha;
    result.pitchQuantized=static_cast<std::int16_t>(clampf(pitch,-1.5f,1.5f)*(32767.0f/1.5f));
    result.rollQuantized=static_cast<std::int16_t>(clampf(roll,-1.5f,1.5f)*(32767.0f/1.5f));
    const float left=previous.leftFootContact()+(current.leftFootContact()-previous.leftFootContact())*alpha;
    const float right=previous.rightFootContact()+(current.rightFootContact()-previous.rightFootContact())*alpha;
    result.leftFootContactByte=static_cast<std::uint8_t>(clampf(left,0.0f,1.0f)*255.0f+0.5f);
    result.rightFootContactByte=static_cast<std::uint8_t>(clampf(right,0.0f,1.0f)*255.0f+0.5f);
    return result;
}

struct HumanReactionVisual {
    float locomotionPhase = 0.0f;
    float locomotionAmount = 0.0f;
    float hitAmount = 0.0f;
    float hitDirectionLocal = 0.0f;
    float vacuumPullAmount = 0.0f;
    float captureCollapseAmount = 0.0f;
    float visibility = 1.0f;
    float soulCubeAmount = 0.0f;
    float attackTimer = 0.0f;
    int attackVariant = 0;
    // Presentation-only projection of the enemy's existing cognition/body
    // state. These values never drive gameplay; they make intent legible.
    float awareness = 0.0f;
    float uncertainty = 1.0f;
    float commitment = 0.0f;
    float disruption = 0.0f;
    float searchAmount = 0.0f;
    float individuality = 0.0f;
};

inline float smoothStep01(float x) {
    x = clampf(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

inline float humanShellThinningAmount(float armor, float armorMax, bool slurpable) {
    if(slurpable||armorMax<=0.001f)return 0.0f;
    const float remaining=clampf(armor/armorMax,0.0f,1.0f);
    // Start exposing deterministic data-shaped holes after the first meaningful
    // damage and grow them into an unmistakable, still-readable silhouette.
    return smoothStep01(clampf((0.86f-remaining)/0.86f,0.0f,1.0f))*0.44f;
}

inline float humanShellTriangleDataSample(std::size_t triangleIndex) {
    std::uint32_t hash=static_cast<std::uint32_t>(triangleIndex)*747796405u+2891336453u;
    hash=((hash>>((hash>>28u)+4u))^hash)*277803737u;
    hash=(hash>>22u)^hash;
    return static_cast<float>(hash&0x00ffffffu)/16777216.0f;
}

inline bool humanShellTriangleMissing(std::size_t triangleIndex,float thinningAmount) {
    return humanShellTriangleDataSample(triangleIndex)<thinningAmount;
}

inline Vec3 humanShellCritCenter() {
    return {0.0f,HUMAN_VISUAL_SPEC.totalHeight-HUMAN_VISUAL_SPEC.headRadius,0.0f};
}

inline float humanShellCritInfluence(const Vec3& triangleCenter) {
    const Vec3 delta=triangleCenter-humanShellCritCenter();
    return smoothStep01(1.0f-clampf(std::sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z)/0.72f,0.0f,1.0f));
}

inline bool humanShellTriangleMissingTowardCrit(std::size_t triangleIndex,float thinningAmount,const Vec3& triangleCenter) {
    const float threshold=clampf(thinningAmount*(0.56f+humanShellCritInfluence(triangleCenter)*1.20f),0.0f,0.72f);
    return humanShellTriangleDataSample(triangleIndex)<threshold;
}

inline Vec3 humanShellAbsorbTowardCrit(const Vec3& vertex,std::size_t triangleIndex,float thinningAmount) {
    if(thinningAmount<=0.001f)return vertex;
    const float damage=clampf(thinningAmount/0.44f,0.0f,1.0f);
    const float channel=0.30f+(1.0f-humanShellTriangleDataSample(triangleIndex))*0.70f;
    return vertex+(humanShellCritCenter()-vertex)*(smoothStep01(damage)*0.075f*channel);
}

inline HumanReactionVisual makeHumanReactionVisual(
    float walkPhase,
    float locomotionAmount,
    float hitFlash,
    float hitDirectionLocal,
    float vacuumPullAmount,
    float captureProgress,
    float soulMorphPhase,
    bool humanVisible,
    float attackTimer = 0.0f,
    int attackVariant = 0,
    float awareness = 0.0f,
    float uncertainty = 1.0f,
    float commitment = 0.0f,
    float disruption = 0.0f,
    float searchAmount = 0.0f,
    float individuality = 0.0f
) {
    HumanReactionVisual visual;
    visual.locomotionPhase = walkPhase;
    visual.locomotionAmount = clampf(locomotionAmount, 0.0f, 1.0f);
    visual.hitAmount = clampf(hitFlash, 0.0f, 1.0f);
    visual.hitDirectionLocal = clampf(hitDirectionLocal, -1.0f, 1.0f);
    visual.vacuumPullAmount = clampf(vacuumPullAmount, 0.0f, 1.0f);
    visual.captureCollapseAmount = smoothStep01(captureProgress);
    visual.visibility = humanVisible ? 1.0f : 0.0f;
    visual.soulCubeAmount = smoothStep01(soulMorphPhase);
    visual.attackTimer = std::max(0.0f, attackTimer);
    visual.attackVariant = std::max(0, std::min(3, attackVariant));
    visual.awareness = clampf(awareness, 0.0f, 1.0f);
    visual.uncertainty = clampf(uncertainty, 0.0f, 1.0f);
    visual.commitment = clampf(commitment, 0.0f, 1.0f);
    visual.disruption = clampf(disruption, 0.0f, 1.0f);
    visual.searchAmount = clampf(searchAmount, 0.0f, 1.0f);
    visual.individuality = clampf(individuality, -1.0f, 1.0f);
    return visual;
}

inline HumanVisualPose makeHumanVisualPose(float yaw, float scale, float time, const HumanReactionVisual& reaction, bool aliveHuman, const HumanBodyPresentation& body = {}) {
    HumanVisualPose pose;
    pose.yaw = yaw + HUMAN_VISUAL_SPEC.forwardYawOffset;
    pose.scale = scale;
    pose.soulMorph = reaction.soulCubeAmount;
    pose.morphShrink = aliveHuman ? (1.0f - pose.soulMorph) : 1.0f;
    const float stress = clampf(reaction.hitAmount * 0.18f + pose.soulMorph, 0.0f, 1.0f);
    pose.scale *= (1.0f - stress * 0.25f) * std::max(0.0f, pose.morphShrink);
    pose.scale *= reaction.visibility;

    const float cadence = reaction.locomotionPhase;
    const float stride = std::sin(cadence);
    const float counterStride = std::sin(cadence + DB_PI);
    const float idle = std::sin(time * 3.0f);
    const float active = aliveHuman ? reaction.locomotionAmount : 0.0f;
    pose.collapse = reaction.captureCollapseAmount;
    pose.vacuumLean = reaction.vacuumPullAmount;
    pose.rootBob = (0.012f * idle + 0.028f * std::abs(stride) * active) * pose.scale;
    pose.torsoPitch = body.authoritative()
        ? body.pitch() - reaction.vacuumPullAmount * 0.12f + pose.collapse * 0.42f
        : -0.04f * active - reaction.hitAmount * 0.16f - reaction.vacuumPullAmount * 0.20f + pose.collapse * 0.42f;
    pose.torsoRoll = body.authoritative()
        ? body.roll()
        : stride * 0.055f * active + reaction.hitDirectionLocal * reaction.hitAmount * 0.18f;
    pose.headPitch = 0.035f * idle - reaction.hitAmount * 0.12f + pose.collapse * 0.24f;
    const float armTrail = reaction.vacuumPullAmount * 0.28f + pose.collapse * 0.42f;
    pose.leftArmSwing = counterStride * 0.46f * active - 0.08f + reaction.hitDirectionLocal * reaction.hitAmount * 0.32f - armTrail;
    pose.rightArmSwing = stride * 0.46f * active - 0.08f - reaction.hitDirectionLocal * reaction.hitAmount * 0.32f - armTrail;
    pose.leftLegSwing = stride * 0.36f * active - pose.collapse * 0.24f;
    pose.rightLegSwing = counterStride * 0.36f * active - pose.collapse * 0.24f;
    pose.hitLean = reaction.hitAmount * 0.08f;
    // Cognition is expressed as body language, not a floating HUD marker.
    // Searchers scan asymmetrically; committed enemies lean into pursuit;
    // disruption visibly steals their balance.
    const float temperament = reaction.individuality;
    const float scan = std::sin(time * (1.35f + std::abs(temperament) * 0.55f)
        + temperament * 2.7f) * reaction.searchAmount;
    const float doubt = reaction.uncertainty * reaction.awareness;
    const float secondaryAttitude=body.authoritative()?0.32f:1.0f;
    pose.torsoPitch -= reaction.commitment * 0.12f * secondaryAttitude;
    pose.torsoRoll += (scan * (0.055f + doubt * 0.055f)
        + temperament * reaction.disruption * 0.10f) * secondaryAttitude;
    pose.headPitch += scan * 0.085f + doubt * 0.035f
        - reaction.commitment * 0.045f;
    pose.leftArmSwing += scan * 0.11f - reaction.disruption * 0.16f;
    pose.rightArmSwing -= scan * 0.11f + reaction.disruption * 0.16f;
    const float supportLoss=body.authoritative()
        ? clampf(1.0f-(body.leftFootContact()+body.rightFootContact())*0.5f,0.0f,1.0f):0.0f;
    pose.leftLegSwing -= reaction.disruption * 0.09f + supportLoss * 0.16f;
    pose.rightLegSwing -= reaction.disruption * 0.09f + supportLoss * 0.16f;
    if(body.authoritative()&&body.fallen()){pose.rootBob-=0.18f*pose.scale;pose.headPitch-=0.20f;}
    const float rubberPulse = reaction.hitAmount * (0.78f + std::sin(time * 23.0f) * 0.22f);
    pose.expressiveScale = {
        1.0f + rubberPulse * 0.10f,
        1.0f - rubberPulse * 0.14f,
        1.0f + rubberPulse * 0.075f
    };

    if (aliveHuman && reaction.attackTimer > 0.0f) {
        const float t = 1.0f - clampf(reaction.attackTimer / HUMAN_SWING_ATTACK_DURATION, 0.0f, 1.0f);
        const float windup = std::sin(clampf(t / HUMAN_SWING_COMMIT_PHASE, 0.0f, 1.0f) * DB_PI * 0.5f) * (t < HUMAN_SWING_COMMIT_PHASE ? 1.0f : 0.0f);
        const float sweepT=clampf((t-HUMAN_SWING_COMMIT_PHASE)/(HUMAN_SWING_END_PHASE-HUMAN_SWING_COMMIT_PHASE),0.0f,1.0f);
        const float strike = std::sin(sweepT * DB_PI);
        const float recover = std::sin(clampf((t - HUMAN_SWING_END_PHASE) / (1.0f-HUMAN_SWING_END_PHASE), 0.0f, 1.0f) * DB_PI);
        const float side = reaction.attackVariant % 2 == 0 ? 1.0f : -1.0f;
        const float low = reaction.attackVariant >= 2 ? 1.0f : 0.0f;
        const float reach = smoothStep01(sweepT);
        pose.torsoPitch += windup * 0.10f - reach * (0.24f + low * 0.08f) + recover * 0.07f;
        pose.torsoRoll += side * (strike * 0.18f - windup * 0.24f - recover * 0.06f);
        pose.headPitch += windup * 0.04f - reach * 0.10f;
        const float lead = reach * (1.34f + low * 0.22f) + strike * 0.34f - windup * 0.74f;
        const float rear = -reach * 0.24f + windup * 0.14f;
        if (side > 0) { pose.rightArmSwing -= lead; pose.leftArmSwing += rear; }
        else { pose.leftArmSwing -= lead; pose.rightArmSwing += rear; }
    }
    return pose;
}

inline HumanVisualPose makeHumanVisualPose(float yaw, float scale, float walkPhase, float time, float hitFlash, float soulMorphPhase, bool aliveHuman) {
    const HumanReactionVisual reaction = makeHumanReactionVisual(walkPhase, aliveHuman ? 1.0f : 0.0f, hitFlash, 0.0f, 0.0f, 0.0f, soulMorphPhase, true);
    return makeHumanVisualPose(yaw, scale, time, reaction, aliveHuman);
}
