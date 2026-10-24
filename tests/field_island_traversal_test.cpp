#include "Game.hpp"
#include "FieldIsland.hpp"

#include <cmath>
#include <cstdio>

namespace {
constexpr float Dt=1.0f/60.0f;

void neutral(Game& game,int frames=1){for(int i=0;i<frames;++i)game.update(Dt);}
bool near(float a,float b,float tolerance=0.04f){return std::abs(a-b)<=tolerance;}
}

int main(){
    Game walk;
    auto& walkState=walk.networkMutableState();
    walkState.player.pos={field_island::MainHalfWidth-0.35f,0.08f,0.0f};
    walkState.player.vel={6.0f,0.0f,0.0f};
    walkState.player.grounded=true;
    neutral(walk,45);
    if(walk.state().player.pos.x<=field_island::MainHalfWidth||walk.state().player.grounded||walk.state().player.pos.y>=-0.2f){
        std::fprintf(stderr,"FIELD_ISLAND_FAIL walkoff pos=(%.3f,%.3f,%.3f) grounded=%d\n",walk.state().player.pos.x,walk.state().player.pos.y,walk.state().player.pos.z,walk.state().player.grounded?1:0);return 1;
    }

    Game jump;
    auto& jumpState=jump.networkMutableState();
    jumpState.player.pos={field_island::MainHalfWidth-0.45f,0.08f,0.0f};
    jumpState.player.vel={6.0f,0.0f,0.0f};
    jump.setTouchControls(0,0,0,0,false,false,true,false,false,false);
    jump.update(Dt);
    jump.setTouchControls(0,0,0,0,false,false,false,false,false,false);
    neutral(jump,20);
    if(jump.state().player.pos.x<=field_island::MainHalfWidth||jump.state().player.grounded||jump.state().player.pos.y<=0.08f){
        std::fprintf(stderr,"FIELD_ISLAND_FAIL jump-off pos=(%.3f,%.3f,%.3f) grounded=%d\n",jump.state().player.pos.x,jump.state().player.pos.y,jump.state().player.pos.z,jump.state().player.grounded?1:0);return 1;
    }

    Game platform;
    auto& platformState=platform.networkMutableState();
    platformState.debug.colliderCount=1;
    RoomCollider& collider=platformState.roomColliders[0];
    collider.minX=14.0f;collider.maxX=16.0f;collider.minZ=-1.0f;collider.maxZ=1.0f;collider.topY=1.5f;
    platformState.player.pos={15.0f,3.0f,0.0f};platformState.player.vel={};platformState.player.jumpVel=-1.0f;platformState.player.grounded=false;
    neutral(platform,90);
    if(!platform.state().player.grounded||!near(platform.state().player.pos.y,1.58f)||platform.state().player.supportIdentity.source!=SupportSource::Collider){
        std::fprintf(stderr,"FIELD_ISLAND_FAIL elevated landing y=%.3f grounded=%d support=%d\n",platform.state().player.pos.y,platform.state().player.grounded?1:0,static_cast<int>(platform.state().player.supportIdentity.source));return 1;
    }

    Game recovery;
    auto& recoveryState=recovery.networkMutableState();
    recoveryState.player.souls=2;
    recoveryState.player.storedSouls[0].id=41;
    recoveryState.captures[0].filled=true;
    recoveryState.captures[0].tokenAwarded=true;
    recoveryState.progression.run.temporaryLevels[0]=2;
    recoveryState.targets[0].alive=true;
    recoveryState.targets[0].armor=1.25f;
    const int roomIndex=recoveryState.roomIndex,roomSeed=recoveryState.roomSeed;
    recoveryState.player.pos={15.0f,field_island::AbyssRecoveryY-0.1f,0.0f};
    recoveryState.player.vel={4.0f,-3.0f,1.0f};recoveryState.player.grounded=false;
    neutral(recovery);
    const auto& recovered=recovery.state();
    if(recovered.player.abyssRecoveryCount!=1||!recovered.player.grounded||!near(recovered.player.pos.x,0.0f)||!near(recovered.player.pos.y,0.08f)||!near(recovered.player.pos.z,15.5f)||
       recovered.player.souls!=2||recovered.player.storedSouls[0].id!=41||!recovered.captures[0].filled||!recovered.captures[0].tokenAwarded||recovered.progression.run.temporaryLevels[0]!=2||!recovered.targets[0].alive||!near(recovered.targets[0].armor,1.25f)||recovered.roomIndex!=roomIndex||recovered.roomSeed!=roomSeed){
        std::fprintf(stderr,"FIELD_ISLAND_FAIL recovery/preservation count=%d pos=(%.3f,%.3f,%.3f) souls=%d capture=%d room=%d/%d\n",recovered.player.abyssRecoveryCount,recovered.player.pos.x,recovered.player.pos.y,recovered.player.pos.z,recovered.player.souls,recovered.captures[0].filled?1:0,recovered.roomSeed,recovered.roomIndex);return 1;
    }

    Game doorway;
    const auto bridgeSupport=doorway.debugPlayerSupportAt(0.0f,-20.0f);
    const auto abyssSupport=doorway.debugPlayerSupportAt(6.0f,-20.0f);
    if(!bridgeSupport.available||abyssSupport.available){
        std::fprintf(stderr,"FIELD_ISLAND_FAIL bridge support=%d side support=%d\n",bridgeSupport.available?1:0,abyssSupport.available?1:0);return 1;
    }

    std::printf("FIELD_ISLAND_PASS walkoff jump elevated recovery preservation bridge\n");
    return 0;
}
