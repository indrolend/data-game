#include "FieldIsland.hpp"
#include "Game.hpp"
#include "gameplay/PhoneBody.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

constexpr float Dt=1.0f/60.0f;

bool near(float a,float b,float epsilon=0.001f){return std::abs(a-b)<=epsilon;}

struct RecoveryTrace { int frame=-1;float greatestX=0.0f;float lowestY=0.0f;Vec3 final{};int count=0; };

RecoveryTrace runWalkOff(){
    Game game;game.debugStartFieldIslandFixture();
    RecoveryTrace trace{};trace.greatestX=game.state().player.pos.x;trace.lowestY=game.state().player.pos.y;
    for(int frame=0;frame<360;++frame){
        game.setTouchControls(1.0f,0.0f,0.0f,0.0f,false,false,false,false,false,false);
        game.update(Dt);
        const auto& player=game.state().player;
        trace.greatestX=std::max(trace.greatestX,player.pos.x);
        trace.lowestY=std::min(trace.lowestY,player.pos.y);
        if(player.abyssRecoveryCount>0){trace.frame=frame;break;}
    }
    trace.final=game.state().player.pos;trace.count=game.state().player.abyssRecoveryCount;
    return trace;
}

} // namespace

int main(){
    Game supportGame;supportGame.reset();
    const float radius=gameplay::PHONE_BODY.supportRadius;
    const auto center=supportGame.debugPlayerSupportAt(0.0f,0.0f);
    const auto beyondEdge=supportGame.debugPlayerSupportAt(field_island::MainHalfWidth+radius+0.01f,0.0f);
    const auto bridge=supportGame.debugPlayerSupportAt(0.0f,field_island::MainHalfDepth+1.0f);
    const auto besideBridge=supportGame.debugPlayerSupportAt(field_island::DoorBridgeHalfWidth+radius+0.01f,field_island::MainHalfDepth+1.0f);
    if(!center.available||center.identity.source!=SupportSource::Ground||beyondEdge.available||
       !bridge.available||besideBridge.available){
        std::fprintf(stderr,"FIELD_ISLAND_SUPPORT_FAIL center=%d edge=%d bridge=%d beside=%d\n",center.available,beyondEdge.available,bridge.available,besideBridge.available);
        return 1;
    }

    Game elevated;elevated.reset();auto& state=elevated.networkMutableState();
    for(auto& collider:state.roomColliders)collider={};state.debug.colliderCount=1;state.slopeSupportCount=0;state.rockSupportCount=0;
    auto& platform=state.roomColliders[0];platform.minX=14.0f;platform.maxX=16.0f;platform.minZ=-1.0f;platform.maxZ=1.0f;platform.bottomY=0.0f;platform.topY=1.0f;
    state.player.pos={15.0f,2.0f,0.0f};state.player.vel={};state.player.jumpVel=-1.0f;state.player.grounded=false;
    for(int frame=0;frame<120&&!elevated.state().player.grounded;++frame){elevated.setTouchControls(0,0,0,0,false,false,false,false,false,false);elevated.update(Dt);}
    const auto elevatedSupport=elevated.debugPlayerSupportAt(15.0f,0.0f);
    if(!elevatedSupport.available||elevatedSupport.identity.source!=SupportSource::Collider||
       !elevated.state().player.grounded||!near(elevated.state().player.pos.y,1.08f)){
        std::fprintf(stderr,"FIELD_ISLAND_ELEVATED_FAIL available=%d source=%d grounded=%d y=%.3f\n",elevatedSupport.available,static_cast<int>(elevatedSupport.identity.source),elevated.state().player.grounded,elevated.state().player.pos.y);
        return 1;
    }

    const RecoveryTrace first=runWalkOff(),second=runWalkOff();
    const bool deterministic=first.frame==second.frame&&near(first.greatestX,second.greatestX)&&near(first.lowestY,second.lowestY)&&
        near(first.final.x,second.final.x)&&near(first.final.y,second.final.y)&&near(first.final.z,second.final.z);
    if(first.count!=1||first.frame<0||first.greatestX<=field_island::MainHalfWidth+0.5f||first.lowestY>=-4.0f||
       !near(first.final.x,0.0f)||!near(first.final.y,0.08f)||!near(first.final.z,15.5f)||!deterministic){
        std::fprintf(stderr,"FIELD_ISLAND_RECOVERY_FAIL frame=%d count=%d max_x=%.3f min_y=%.3f final=(%.3f,%.3f,%.3f) deterministic=%d\n",first.frame,first.count,first.greatestX,first.lowestY,first.final.x,first.final.y,first.final.z,deterministic);
        return 1;
    }
    std::printf("FIELD_ISLAND_TRAVERSAL_OK finite_ground=YES walk_off=YES elevated_support=YES doorway_bridge=YES recovery_frame=%d max_x=%.3f min_y=%.3f deterministic=MATCH\n",first.frame,first.greatestX,first.lowestY);
    return 0;
}
