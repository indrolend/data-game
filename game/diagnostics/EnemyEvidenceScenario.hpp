#pragma once

#include <algorithm>
#include <cmath>

#include "Game.hpp"

namespace evidence {

inline constexpr int EnemyObstructionTicks = 1200;
inline constexpr float EnemyRadius = 0.5f;

inline void configureEnemyObstruction(Game& game) {
    game.reset();
    GameState& state=game.networkMutableState();
    state.started=true;
    state.uiPaused=false;
    state.attractMode=false;
    state.roomClear=true;
    state.requiredSouls=0;
    state.depositedSouls=0;
    state.cinematic.introActive=false;
    state.upgradeMenu.active=false;
    state.localSettings.graphicsPreset=1;
    state.localSettings.shadows=true;
    state.localSettings.portalWindow=false;
    state.localSettings.particles=false;
    state.localSettings.fpsCounter=false;
    state.localSettings.mobileFraming=false;
    for(auto& target:state.targets)target=TargetState{};
    for(auto& collider:state.roomColliders)collider=RoomCollider{};

    RoomCollider& obstruction=state.roomColliders[0];
    obstruction.minX=-1.5f;obstruction.maxX=1.5f;
    obstruction.minZ=-1.5f;obstruction.maxZ=1.5f;
    obstruction.bottomY=0.0f;obstruction.topY=2.2f;
    obstruction.width=3.0f;obstruction.depth=3.0f;obstruction.height=2.2f;
    obstruction.center={0.0f,1.1f,0.0f};
    state.debug.colliderCount=0;

    state.player.pos={5.0f,0.08f,0.0f};
    state.player.vel={};
    state.player.grounded=true;
    state.player.battery=100.0f;

    TargetState& enemy=state.targets[0];
    enemy.alive=true;
    enemy.slurpable=false;
    enemy.pos={-5.0f,0.08f,0.0f};
    enemy.walkTarget=state.player.pos;
    enemy.armor=2.0f;
    enemy.health=1.0f;
    enemy.attackCooldown=999.0f;
    enemy.visibility=1.0f;
    enemy.visualYaw=-DB_PI*0.5f;
    state.camera.firstPerson=false;

    // Establish a confirmed view before introducing the obstruction. The
    // scenario then proves locomotion around a blocker using remembered
    // evidence rather than granting sight through it.
    for(int frame=0;frame<120;++frame){
        game.update(1.0f/60.0f);
        enemy.pos={-5.0f,0.08f,0.0f};enemy.vel={};enemy.walkTarget=state.player.pos;
    }
    state.debug.colliderCount=1;
}

inline bool finite(const Vec3& value) {
    return std::isfinite(value.x)&&std::isfinite(value.y)&&std::isfinite(value.z);
}

inline bool overlapsObstruction(const TargetState& enemy,const RoomCollider& collider) {
    return enemy.pos.x>collider.minX-EnemyRadius&&enemy.pos.x<collider.maxX+EnemyRadius&&
        enemy.pos.z>collider.minZ-EnemyRadius&&enemy.pos.z<collider.maxZ+EnemyRadius&&
        enemy.pos.y<collider.topY;
}

inline float obstructionClearance(const TargetState& enemy,const RoomCollider& collider) {
    const float dx=std::max({collider.minX-enemy.pos.x,0.0f,enemy.pos.x-collider.maxX});
    const float dz=std::max({collider.minZ-enemy.pos.z,0.0f,enemy.pos.z-collider.maxZ});
    return std::sqrt(dx*dx+dz*dz)-EnemyRadius;
}

struct EnemyObstructionObservation {
    int tick=0;
    Vec3 position{};
    Vec3 velocity{};
    Vec3 goal{};
    Vec3 supportNormal{0.0f,1.0f,0.0f};
    int supportSource=0;
    float goalDistance=0.0f;
    float speed=0.0f;
    float progress=0.0f;
    float obstructionClearance=0.0f;
    float maximumDetour=0.0f;
    float perceptionConfidence=0.0f;
    float perceptionUncertainty=1.0f;
    gameplay::EnemyBehaviorMode cognition=gameplay::EnemyBehaviorMode::Rest;
    int stalledTicks=0;
    bool colliderOverlap=false;
    bool attackActive=false;
    bool attackHit=false;
    bool perceptionConfirmed=false;
    bool hasSpatialBelief=false;
    bool finiteValues=true;
};

class EnemyObstructionTracker {
public:
    EnemyObstructionObservation observe(const Game& game,int tick) {
        const GameState& state=game.state();
        const TargetState& enemy=state.targets[0];
        const RoomCollider& collider=state.roomColliders[0];
        const Vec3 travel=enemy.pos-previousPosition_;
        const float travelled=horizontalLength(travel);
        stalledTicks_=travelled<0.0001f?stalledTicks_+1:0;
        maximumDetour_=std::max(maximumDetour_,std::abs(enemy.pos.z));
        const auto support=game.debugWorldSupportAt(enemy.pos.x,enemy.pos.z,EnemyRadius);
        EnemyObstructionObservation result;
        result.tick=tick;
        result.position=enemy.pos;
        result.velocity=enemy.vel;
        result.goal=state.player.pos;
        result.supportNormal=support.normal;
        result.supportSource=static_cast<int>(support.identity.source);
        result.goalDistance=horizontalLength(Vec3{state.player.pos.x-enemy.pos.x,0.0f,state.player.pos.z-enemy.pos.z});
        result.speed=horizontalLength(enemy.vel);
        result.progress=enemy.pos.x-initialPosition_.x;
        result.obstructionClearance=evidence::obstructionClearance(enemy,collider);
        result.maximumDetour=maximumDetour_;
        result.stalledTicks=stalledTicks_;
        result.colliderOverlap=overlapsObstruction(enemy,collider);
        result.attackActive=enemy.attackTimer>0.0f;
        result.attackHit=enemy.attackHit;
        const auto& perception=game.enemyPerceptions()[0];
        result.cognition=game.enemyBehaviors()[0].mode;
        result.perceptionConfidence=perception.confidence;
        result.perceptionUncertainty=perception.uncertainty;
        result.perceptionConfirmed=perception.confirmed;
        result.hasSpatialBelief=perception.confidence>0.035f;
        result.finiteValues=finite(enemy.pos)&&finite(enemy.vel)&&finite(support.normal)&&
            std::isfinite(result.goalDistance)&&std::isfinite(result.obstructionClearance)&&
            std::isfinite(result.perceptionConfidence)&&std::isfinite(result.perceptionUncertainty);
        previousPosition_=enemy.pos;
        return result;
    }

    void reset(const Game& game) {
        initialPosition_=previousPosition_=game.state().targets[0].pos;
        maximumDetour_=0.0f;
        stalledTicks_=0;
    }

private:
    Vec3 initialPosition_{};
    Vec3 previousPosition_{};
    float maximumDetour_=0.0f;
    int stalledTicks_=0;
};

inline const char* behaviorMode(const EnemyObstructionObservation& observation) {
    if(observation.attackActive)return "attack";
    if(observation.colliderOverlap)return "invalid_overlap";
    if(observation.goalDistance<2.0f)return "arrived";
    if(observation.stalledTicks>=30)return "stalled";
    if(observation.obstructionClearance<0.75f)return "routing_obstruction";
    return "pursuit";
}

inline const char* cognitionMode(gameplay::EnemyBehaviorMode mode) {
    switch(mode) {
        case gameplay::EnemyBehaviorMode::Rest:return "rest";
        case gameplay::EnemyBehaviorMode::Orient:return "orient";
        case gameplay::EnemyBehaviorMode::Investigate:return "investigate";
        case gameplay::EnemyBehaviorMode::Alert:return "alert";
        case gameplay::EnemyBehaviorMode::Engage:return "engage";
        case gameplay::EnemyBehaviorMode::Search:return "search";
    }
    return "unknown";
}

} // namespace evidence
