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
    bool ok=!target.alive&&state.player.souls==1;
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
