#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <string>

#include "Game.hpp"
#include "SignalResidue.hpp"

struct SignalResidueLifecycleAccess {
    static void update(Game& game,float dt) { game.updateParticles(dt); }
};

namespace {
constexpr float Dt=1.0f/60.0f;

bool near(float a,float b,float epsilon=0.0005f){return std::fabs(a-b)<=epsilon;}

void emit(const char* name,int tick,const ParticleState& before,const ParticleState& after,
          float batteryBefore,float batteryAfter,bool vacuumActive,const Vec3& pullPoint,float supportHeight,
          bool candidateAttracted,bool nativeAttracted){
    if(candidateAttracted!=nativeAttracted){
        std::fprintf(stderr,"NDRA_ATTRACTION_MISMATCH case=%s tick=%d expected=%d actual=%d\n",
            name,tick,static_cast<int>(candidateAttracted),static_cast<int>(nativeAttracted));
        std::exit(EXIT_FAILURE);
    }
    const Vec3 delta=pullPoint-before.pos;
    const float distance=length(delta);
    std::printf("{\"schema_version\":\"ndra.trace/v1\",\"case\":\"%s\",\"tick\":%d,"
        "\"dt\":%.9g,\"material\":%u,\"vacuum_active\":%s,\"battery_before\":%.9g,\"battery_after\":%.9g,"
        "\"support_height\":%.9g,\"pull_point\":[%.9g,%.9g,%.9g],\"distance\":%.9g,"
        "\"pre\":{\"position\":[%.9g,%.9g,%.9g],\"velocity\":[%.9g,%.9g,%.9g],\"life\":%.9g,\"surface_flags\":%u},"
        "\"post\":{\"position\":[%.9g,%.9g,%.9g],\"velocity\":[%.9g,%.9g,%.9g],\"life\":%.9g,\"surface_flags\":%u},"
        "\"candidate_attracted\":%s,\"native_attracted\":%s,\"captured\":%s}\n",
        name,tick,Dt,static_cast<unsigned>(before.material),vacuumActive?"true":"false",batteryBefore,batteryAfter,
        supportHeight,pullPoint.x,pullPoint.y,pullPoint.z,distance,
        before.pos.x,before.pos.y,before.pos.z,before.vel.x,before.vel.y,before.vel.z,before.life,static_cast<unsigned>(before.surfaceFlags),
        after.pos.x,after.pos.y,after.pos.z,after.vel.x,after.vel.y,after.vel.z,after.life,static_cast<unsigned>(after.surfaceFlags),
        candidateAttracted?"true":"false",nativeAttracted?"true":"false",batteryAfter>batteryBefore?"true":"false");
}

void runCase(const char* name,float y,bool vacuum,float battery,ParticleMaterial material,float pullX,float life=3.0f){
    Game game; game.reset();
    GameState& state=game.networkMutableState();
    for(auto& particle:state.particles)particle=ParticleState{};
    state.vacuum.active=vacuum;state.player.battery=battery;state.phoneTransform.vacuumPullPoint={pullX,y,0.0f};
    ParticleState& particle=state.particles[0];particle.material=material;particle.life=particle.maxLife=life;
    particle.size=0.14f;particle.pos={0.0f,y,0.0f};particle.vel={};
    const ParticleState before=particle;const float batteryBefore=state.player.battery;
    const float distance=std::fabs(pullX);
    const bool candidate=vacuum&&material==ParticleMaterial::SignalResidue&&battery<99.95f&&life>0.0f&&distance<=signal_residue::AttractionRadius;
    SignalResidueLifecycleAccess::update(game,Dt);
    const bool native=std::fabs(particle.vel.x)>0.001f||particle.life<=0.0f;
    const float support=game.debugWorldSupportAt(before.pos.x,before.pos.z,before.size*0.35f).height;
    emit(name,0,before,particle,batteryBefore,state.player.battery,vacuum,state.phoneTransform.vacuumPullPoint,support,candidate,native);
}

void runSlopeCase(){
    Game game;game.debugStartSlopeLab();GameState& state=game.networkMutableState();
    for(auto& particle:state.particles)particle=ParticleState{};
    const SlopeSupport slope=state.slopeSupports[0];
    const float x=(slope.minX+slope.maxX)*0.5f,z=(slope.minZ+slope.maxZ)*0.5f;
    const float support=game.debugWorldSupportAt(x,z,0.049f).height;
    state.vacuum.active=true;state.player.battery=20.0f;state.phoneTransform.vacuumPullPoint={x+0.5f,support+0.025f,z};
    ParticleState& particle=state.particles[0];particle.material=ParticleMaterial::SignalResidue;particle.life=particle.maxLife=3.0f;particle.size=0.14f;particle.pos={x,support+0.025f,z};
    const ParticleState before=particle;SignalResidueLifecycleAccess::update(game,Dt);
    const bool native=std::fabs(particle.vel.x)>0.001f||particle.life<=0.0f;
    emit("control_slope_geometry",0,before,particle,20.0f,state.player.battery,true,state.phoneTransform.vacuumPullPoint,support,true,native);
}
}

int main(){
    runCase("A_grounded",0.105f,true,20.0f,ParticleMaterial::SignalResidue,0.5f);
    runCase("B_airborne",0.8f,true,20.0f,ParticleMaterial::SignalResidue,0.5f);
    runCase("control_vacuum_off",0.105f,false,20.0f,ParticleMaterial::SignalResidue,0.5f);
    runCase("control_outside_radius",0.105f,true,20.0f,ParticleMaterial::SignalResidue,signal_residue::AttractionRadius+0.5f);
    runCase("control_battery_threshold",0.105f,true,99.95f,ParticleMaterial::SignalResidue,0.5f);
    runCase("control_non_residue",0.105f,true,20.0f,ParticleMaterial::Environment,0.5f);
    runCase("control_short_life",0.105f,true,20.0f,ParticleMaterial::SignalResidue,0.5f,Dt*0.5f);
    runCase("control_capture_radius",0.105f,true,20.0f,ParticleMaterial::SignalResidue,0.01f);
    runSlopeCase();

    Game transition;transition.reset();GameState& state=transition.networkMutableState();
    for(auto& particle:state.particles)particle=ParticleState{};
    state.vacuum.active=true;state.player.battery=20.0f;state.phoneTransform.vacuumPullPoint={0.5f,1.0f,0.0f};
    ParticleState& particle=state.particles[0];particle.material=ParticleMaterial::SignalResidue;particle.life=particle.maxLife=3.0f;particle.size=0.14f;particle.pos={0.0f,0.105f,0.0f};
    for(int tick=0;tick<2;++tick){
        const ParticleState before=particle;const float batteryBefore=state.player.battery;
        SignalResidueLifecycleAccess::update(transition,Dt);
        const bool native=std::fabs(particle.vel.x)>0.001f&&near(particle.vel.x,before.vel.x,0.001f)==false;
        const float support=transition.debugWorldSupportAt(before.pos.x,before.pos.z,before.size*0.35f).height;
        emit("C_grounded_to_airborne",tick,before,particle,batteryBefore,state.player.battery,true,state.phoneTransform.vacuumPullPoint,support,true,native);
    }
    return 0;
}
