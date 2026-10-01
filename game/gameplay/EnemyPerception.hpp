#pragma once

#include <algorithm>
#include <cmath>

#include "Math.hpp"

namespace gameplay {

struct EnemyPerceptionState {
    Vec3 lastSeenPosition{};
    Vec3 lastSeenVelocity{};
    float confidence = 0.0f;
    float uncertainty = 1.0f;
    float headYaw = 0.0f;
    float headPitch = 0.0f;
    float searchPhase = 0.0f;
    bool confirmed = false;
};

struct EnemyPerceptionInput {
    Vec3 observerPosition{};
    Vec3 targetPosition{};
    Vec3 targetVelocity{};
    float bodyYaw = 0.0f;
    float bodyPitch = 0.0f;
    float bodyRoll = 0.0f;
    float visibility = 0.0f;
    float vagueAwareness = 0.0f;
    float individuality = 0.0f;
    float dt = 0.0f;
    bool sampled = false;
};

struct EnemyPerceptionOutput {
    Vec3 believedPosition{};
    Vec3 perceivedVelocity{};
    Vec3 headDirection{0.0f,0.0f,-1.0f};
    float confidence = 0.0f;
    float uncertainty = 1.0f;
    float headYaw = 0.0f;
    float headPitch = 0.0f;
    bool confirmed = false;
    bool hasSpatialBelief = false;
};

inline float finitePerceptionValue(float value,float fallback=0.0f){return std::isfinite(value)?value:fallback;}
inline Vec3 finitePerceptionVector(const Vec3& value){return {finitePerceptionValue(value.x),finitePerceptionValue(value.y),finitePerceptionValue(value.z)};}
inline float wrapPerceptionAngle(float angle){
    angle=finitePerceptionValue(angle);
    while(angle>DB_PI)angle-=DB_PI*2.0f;
    while(angle<-DB_PI)angle+=DB_PI*2.0f;
    return angle;
}

inline bool perceptionSegmentHitsBox(const Vec3& start,const Vec3& end,const Vec3& minimum,const Vec3& maximum,float& entry){
    const Vec3 a=finitePerceptionVector(start),b=finitePerceptionVector(end),delta=b-a;
    float nearT=0.0f,farT=1.0f;
    const float origins[3]={a.x,a.y,a.z},directions[3]={delta.x,delta.y,delta.z};
    const float minima[3]={minimum.x,minimum.y,minimum.z},maxima[3]={maximum.x,maximum.y,maximum.z};
    for(int axis=0;axis<3;++axis){
        if(std::abs(directions[axis])<0.00001f){if(origins[axis]<minima[axis]||origins[axis]>maxima[axis])return false;continue;}
        float lo=(minima[axis]-origins[axis])/directions[axis],hi=(maxima[axis]-origins[axis])/directions[axis];
        if(lo>hi)std::swap(lo,hi);
        nearT=std::max(nearT,lo);farT=std::min(farT,hi);
        if(nearT>farT)return false;
    }
    entry=nearT;
    return farT>=0.0f&&nearT<=1.0f;
}

inline float visualAcquisitionStrength(float distance,float forwardDot,float targetSpeed,float transmission){
    distance=std::max(0.0f,finitePerceptionValue(distance,1000.0f));
    forwardDot=std::max(-1.0f,std::min(1.0f,finitePerceptionValue(forwardDot,-1.0f)));
    targetSpeed=std::max(0.0f,std::min(12.0f,finitePerceptionValue(targetSpeed)));
    transmission=std::max(0.0f,std::min(1.0f,finitePerceptionValue(transmission)));
    if(transmission<=0.0f)return 0.0f;
    const float angular=forwardDot>0.35f?1.0f:(forwardDot>-0.45f?0.48f:0.12f);
    const float motionAttraction=forwardDot<0.35f?std::min(0.32f,targetSpeed*0.055f):0.0f;
    const float range=forwardDot>0.35f?18.0f:(forwardDot>-0.45f?7.5f:3.0f);
    return std::max(0.0f,std::min(1.0f,(angular+motionAttraction)*std::max(0.0f,1.0f-distance/range)*transmission));
}

inline EnemyPerceptionOutput updateEnemyPerception(const EnemyPerceptionInput& raw,EnemyPerceptionState& state){
    const float dt=std::max(0.0f,std::min(0.1f,finitePerceptionValue(raw.dt)));
    state.lastSeenPosition=finitePerceptionVector(state.lastSeenPosition);
    state.lastSeenVelocity=finitePerceptionVector(state.lastSeenVelocity);
    state.confidence=std::max(0.0f,std::min(1.0f,finitePerceptionValue(state.confidence)));
    state.uncertainty=std::max(0.0f,std::min(1.0f,finitePerceptionValue(state.uncertainty,1.0f)));
    state.headYaw=std::max(-1.18f,std::min(1.18f,finitePerceptionValue(state.headYaw)));
    state.headPitch=std::max(-0.48f,std::min(0.48f,finitePerceptionValue(state.headPitch)));
    state.searchPhase=finitePerceptionValue(state.searchPhase);
    const Vec3 observer=finitePerceptionVector(raw.observerPosition);
    const float visibility=std::max(0.0f,std::min(1.0f,finitePerceptionValue(raw.visibility)));
    if(raw.sampled){
        if(visibility>=0.18f){
            state.lastSeenPosition=finitePerceptionVector(raw.targetPosition);
            state.lastSeenVelocity=finitePerceptionVector(raw.targetVelocity);
            state.confidence=std::max(state.confidence,visibility);
            state.uncertainty=std::max(0.0f,1.0f-visibility);
            state.confirmed=visibility>=0.26f;
        }else state.confirmed=false;
    }
    if(!state.confirmed){
        // Memory must outlive a deliberate route around one room-scale
        // obstruction. It still decays to zero, but not faster than the
        // current physical body can express the remembered intention.
        state.confidence=std::max(0.0f,state.confidence-dt*(0.07f+state.uncertainty*0.06f));
        state.uncertainty=std::min(1.0f,state.uncertainty+dt*0.13f);
        state.lastSeenVelocity=state.lastSeenVelocity*std::max(0.0f,1.0f-dt*2.5f);
        state.searchPhase=std::fmod(state.searchPhase+dt*(1.4f+state.uncertainty*2.3f),DB_PI*2.0f);
    }else{
        state.confidence=std::min(1.0f,state.confidence+dt*0.35f);
        state.uncertainty=std::max(0.0f,state.uncertainty-dt*0.45f);
    }
    const bool hasBelief=state.confidence>0.035f;
    Vec3 interest=hasBelief?state.lastSeenPosition-observer:Vec3{std::sin((0.65f+finitePerceptionValue(raw.individuality)*0.17f)*(state.uncertainty*5.0f+state.confidence*2.0f)),0.0f,-1.0f};
    const float horizontal=std::sqrt(interest.x*interest.x+interest.z*interest.z);
    float desiredYaw=state.headYaw,desiredPitch=0.0f;
    if(horizontal>0.001f){
        desiredYaw=wrapPerceptionAngle(std::atan2(-interest.x,-interest.z)-finitePerceptionValue(raw.bodyYaw));
        desiredPitch=-std::atan2(interest.y,horizontal)-finitePerceptionValue(raw.bodyPitch)*0.35f;
    }
    if(hasBelief&&!state.confirmed){
        const float nearby=1.0f-std::min(1.0f,horizontal/4.0f);
        desiredYaw+=std::sin(state.searchPhase+finitePerceptionValue(raw.individuality)*1.7f)*(0.18f+nearby*0.52f)*state.uncertainty;
        desiredPitch+=std::sin(state.searchPhase*0.63f+1.1f)*0.10f*state.uncertainty;
    }
    const float awareness=std::max(0.0f,std::min(1.0f,finitePerceptionValue(raw.vagueAwareness)));
    if(!hasBelief&&awareness>0.0f)desiredYaw+=std::sin((state.uncertainty+1.0f)*4.7f+raw.individuality)*0.55f*awareness;
    desiredYaw=std::max(-1.18f,std::min(1.18f,desiredYaw-finitePerceptionValue(raw.bodyRoll)*0.20f));
    desiredPitch=std::max(-0.48f,std::min(0.48f,desiredPitch));
    const float follow=1.0f-std::exp(-dt*(state.confirmed?9.0f:4.0f));
    state.headYaw+=wrapPerceptionAngle(desiredYaw-state.headYaw)*follow;
    state.headPitch+=(desiredPitch-state.headPitch)*follow;
    const float worldYaw=finitePerceptionValue(raw.bodyYaw)+state.headYaw;
    EnemyPerceptionOutput out;
    out.believedPosition=state.lastSeenPosition;out.perceivedVelocity=state.confirmed?state.lastSeenVelocity:Vec3{};
    out.headDirection={-std::sin(worldYaw)*std::cos(state.headPitch),-std::sin(state.headPitch),-std::cos(worldYaw)*std::cos(state.headPitch)};
    out.confidence=state.confidence;out.uncertainty=state.uncertainty;out.headYaw=state.headYaw;out.headPitch=state.headPitch;
    out.confirmed=state.confirmed;out.hasSpatialBelief=hasBelief;
    return out;
}

} // namespace gameplay
