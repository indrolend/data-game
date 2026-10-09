#include <cmath>
#include <cstdio>

#include "Game.hpp"
#include "SignalResidue.hpp"

struct SignalResidueLifecycleAccess {
    static void capture(Game& game,int index) { game.captureSoul(index); }
    static void update(Game& game,float dt) { game.updateParticles(dt); }
};

namespace {

bool near(float a,float b,float epsilon=0.001f) { return std::fabs(a-b)<=epsilon; }

}  // namespace

int main() {
    Game surfaceGame;
    surfaceGame.debugStartSlopeLab();
    GameState& surfaceState=surfaceGame.networkMutableState();
    for(auto& particle:surfaceState.particles)particle=ParticleState{};
    const SlopeSupport slope=surfaceState.slopeSupports[0];
    ParticleState& surfaceResidue=surfaceState.particles[0];
    surfaceResidue.material=ParticleMaterial::SignalResidue;
    surfaceResidue.life=surfaceResidue.maxLife=signal_residue::LifetimeSeconds;
    surfaceResidue.size=0.14f;
    surfaceResidue.pos={(slope.minX+slope.maxX)*0.5f,2.0f,(slope.minZ+slope.maxZ)*0.5f};
    for(int frame=0;frame<180&&!particleSurfaceSettled(surfaceResidue);++frame)
        SignalResidueLifecycleAccess::update(surfaceGame,1.0f/60.0f);
    const auto expectedSurface=surfaceGame.debugWorldSupportAt(
        surfaceResidue.pos.x,surfaceResidue.pos.z,surfaceResidue.size*0.35f);
    const bool surfaceAttached=particleSurfaceSettled(surfaceResidue)
        &&near(surfaceResidue.pos.y,expectedSurface.height+0.025f,0.002f)
        &&dot3(particleSurfaceNormal(surfaceResidue),expectedSurface.normal)>0.995f;

    Game game;
    game.reset();
    GameState& state=game.networkMutableState();
    for(auto& target:state.targets)target=TargetState{};
    for(auto& particle:state.particles)particle=ParticleState{};
    state.nextParticle=0;
    state.player.battery=20.0f;
    state.player.souls=0;
    TargetState& target=state.targets[0];
    target.alive=true;
    target.slurpable=true;
    target.pos={0.0f,1.0f,0.0f};

    SignalResidueLifecycleAccess::capture(game,0);
    int residueCount=0;
    for(const auto& particle:state.particles)
        if(particle.life>0.0f&&particle.material==ParticleMaterial::SignalResidue)++residueCount;
    bool ok=surfaceAttached&&!target.alive&&state.player.souls==1;
    ok&=near(state.player.battery,20.0f+signal_residue::ImmediateCaptureCharge);
    ok&=residueCount==signal_residue::FragmentCount;

    state.vacuum.active=true;
    state.player.battery=30.0f;
    state.phoneTransform.vacuumPullPoint={0.0f,0.025f,0.0f};
    int offset=0;
    for(auto& particle:state.particles)if(particle.life>0.0f&&particle.material==ParticleMaterial::SignalResidue){
        particle.pos={0.01f*static_cast<float>(offset++),0.025f,0.0f};
        particle.vel={};
    }
    SignalResidueLifecycleAccess::update(game,1.0f/60.0f);
    int remaining=0;
    for(const auto& particle:state.particles)
        if(particle.life>0.0f&&particle.material==ParticleMaterial::SignalResidue)++remaining;
    ok&=remaining==0;
    ok&=near(state.player.battery,30.0f+signal_residue::TotalRecoverableCharge);

    if(!ok){
        std::fprintf(stderr,"SIGNAL_RESIDUE_FAILED fragments=%d remaining=%d battery=%.3f\n",
            residueCount,remaining,state.player.battery);
        return 1;
    }
    std::printf("SIGNAL_RESIDUE_OK immediate=%.1f recoverable=%.1f fragments=%d\n",
        signal_residue::ImmediateCaptureCharge,signal_residue::TotalRecoverableCharge,residueCount);
    return 0;
}
