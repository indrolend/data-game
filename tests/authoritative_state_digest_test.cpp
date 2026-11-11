#include "Game.hpp"
#include "MultiplayerProtocol.hpp"

#include <cstdio>
#include <memory>
#include <string>

namespace {
bool expect(bool condition,const char* label){
    if(!condition)std::fprintf(stderr,"AUTHORITATIVE_STATE_DIGEST_FAIL check=%s\n",label);
    return condition;
}
void drive(Game& game,int frame){
    const bool vacuum=frame>=90&&frame<150;
    game.setTouchControls(frame<80?0.25f:0.0f,frame<120?0.70f:0.0f,
        frame<30?0.35f:0.0f,0.0f,vacuum,frame<60,
        frame==12,frame==72,frame==170,false);
    game.update(1.0f/60.0f);
}
}

int main(){
    bool ok=true;
    auto first=std::make_unique<Game>();
    auto second=std::make_unique<Game>();
    first->debugStartGeneratedRoomFixture(424242,3);
    second->debugStartGeneratedRoomFixture(424242,3);
    ok&=expect(dbnet::authoritativeStateDigest(first->state())==
               dbnet::authoritativeStateDigest(second->state()),"same_fixture");
    for(int frame=0;frame<240;++frame){
        drive(*first,frame);drive(*second,frame);
        ok&=expect(dbnet::authoritativeStateDigest(first->state())==
                   dbnet::authoritativeStateDigest(second->state()),"deterministic_stream");
    }
    const std::uint64_t base=dbnet::authoritativeStateDigest(first->state());
    const std::string hex=dbnet::authoritativeStateDigestHex(first->state());
    ok&=expect(hex.size()==16,"fixed_width_hex");
    auto presentation=std::make_unique<GameState>(first->state());
    presentation->localSettings.graphicsPreset=(presentation->localSettings.graphicsPreset+1)%3;
    presentation->localSettings.shadows=!presentation->localSettings.shadows;
    presentation->camera.pos.x+=4.0f;presentation->camera.lookTarget.y+=2.0f;
    presentation->camera.verticalFovDegrees=42.0f;presentation->phoneVisual.actionLift+=0.75f;
    presentation->hud.crosshairOpacity=1.0f-presentation->hud.crosshairOpacity;
    presentation->cinematic.textInteraction+=0.5f;presentation->particles[0].life+=3.0f;
    ok&=expect(dbnet::authoritativeStateDigest(*presentation)==base,"presentation_isolation");
    auto gameplay=std::make_unique<GameState>(first->state());
    gameplay->player.battery-=1.0f;ok&=expect(dbnet::authoritativeStateDigest(*gameplay)!=base,"battery_mutation");
    *gameplay=first->state();gameplay->player.pos.x+=1.0f;ok&=expect(dbnet::authoritativeStateDigest(*gameplay)!=base,"player_mutation");
    *gameplay=first->state();gameplay->targets[0].armor-=0.5f;ok&=expect(dbnet::authoritativeStateDigest(*gameplay)!=base,"target_mutation");
    *gameplay=first->state();gameplay->roomIndex+=1;ok&=expect(dbnet::authoritativeStateDigest(*gameplay)!=base,"room_mutation");
    *gameplay=first->state();gameplay->progression.run.temporaryLevels[0]+=1;ok&=expect(dbnet::authoritativeStateDigest(*gameplay)!=base,"progression_mutation");
    if(ok)std::printf("AUTHORITATIVE_STATE_DIGEST_OK digest=%s frames=240 presentation_isolated=1 gameplay_sensitive=1\n",hex.c_str());
    return ok?0:1;
}
