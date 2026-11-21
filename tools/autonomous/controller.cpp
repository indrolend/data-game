#define main old_audit_main
#include "audit_support.cpp"
#undef main
#include "world/RoomGeometry.hpp"
#include <iostream>
#include <array>
struct Metrics {int deaths=0,captures=0,shots=0,deposits=0,clears=0,rooms=0,alive=0;float nearest=0;};
static float angle(float a){while(a>3.14159265f)a-=6.2831853f;while(a< -3.14159265f)a+=6.2831853f;return a;}
static void control(Game&g,int tick){const auto&s=g.state();const auto&p=s.player;const auto&c=s.camera;
 const TargetState* target=nullptr;float best=1e12f;
 for(const auto&t:s.targets)if(t.alive){float dx=t.pos.x-p.pos.x,dz=t.pos.z-p.pos.z,d=dx*dx+dz*dz+(t.slurpable?0.f:25.f);if(d<best){best=d;target=&t;}}
 bool depositing=p.souls>0;float tx=0,tz=0,ty=0;bool vacuum=false,melee=false,shoot=false,sprint=false;
 if(depositing){int slot=-1;for(int i=0;i<s.requiredSouls;i++)if(!s.captures[i].filled){slot=i;break;}if(slot>=0){tx=s.captures[slot].pos.x;ty=s.captures[slot].pos.y;tz=s.captures[slot].pos.z+(s.topology.currentTileIndex * world::RoomDepth);shoot=(tick%24==0);}}
 else if(target){tx=target->pos.x;ty=target->pos.y+0.45f;tz=target->pos.z;vacuum=target->slurpable;melee=!target->slurpable&&std::hypot(tx-p.pos.x,tz-p.pos.z)<5.5f&&tick%22==0;sprint=!vacuum;}
 float dx=tx-p.pos.x,dz=tz-p.pos.z,d=std::hypot(dx,dz);float yaw=std::atan2(-dx,-dz);float yawDiff=angle(yaw-c.yaw);float lookX=-std::clamp(yawDiff,-0.16f,0.16f)/0.003f;
 // The projectile originates at phone-screen height; pitch toward the capture slots.
 float pitch=std::atan2(ty-(p.pos.y+0.55f),std::max(d,0.1f));float lookY=-std::clamp(pitch-c.pitch,-0.07f,0.07f)/0.003f;
 float forward=0,strafe=0;if(target&&!depositing){float stop=vacuum?1.25f:2.2f;if(d>stop){forward=std::clamp((d-stop)*0.7f,0.f,1.f);if(std::abs(yawDiff)>0.55f)forward*=0.35f;}}
 if(depositing){ // fire only once aimed; stop moving to avoid missing wall sockets
 shoot=shoot&&std::abs(yawDiff)<0.07f&&std::abs(pitch-c.pitch)<0.07f;
 }
 g.setTouchControls(strafe,forward,lookX,lookY,vacuum,sprint,false,melee,shoot,false);g.update(1.f/60.f);
}
int main(int argc,char**argv){int count=argc>1?atoi(argv[1]):4;int ticks=argc>2?atoi(argv[2]):3600;if(count<1||count>64||ticks<1||ticks>3600)return 2;std::string dir=argc>3?argv[3]:"/mnt/data/native-controller";std::filesystem::create_directories(dir);std::ofstream csv(dir+"/native_trials.csv");csv<<"seed,ticks,enemy_deaths,soul_captures,shots,deposits,room_clears,room_transitions,final_souls,final_deposits,final_room,final_battery,final_digest\n";
 for(int k=0;k<count;k++){int seed=12345+k*7919;auto g=std::make_unique<Game>();g->reset();g->debugStartGeneratedRoomFixture(seed,2);Metrics m;for(int i=0;i<ticks;i++){const auto before=g->state();control(*g,i);const auto&after=g->state();int ba=0,aa=0;for(auto&t:before.targets)ba+=t.alive;for(auto&t:after.targets)aa+=t.alive;m.deaths+=std::max(0,ba-aa);m.captures+=std::max(0,after.player.souls-before.player.souls);m.shots+=std::max(0,before.player.souls-after.player.souls);m.deposits+=std::max(0,after.depositedSouls-before.depositedSouls);m.clears+=!before.roomClear&&after.roomClear;m.rooms+=before.roomIndex!=after.roomIndex;auto violation=check(*g);if(!violation.empty()){std::cerr<<"VIOLATION seed="<<seed<<" tick="<<i<<" reason="<<violation<<"\n";return 1;}}const auto&s=g->state();csv<<seed<<','<<ticks<<','<<m.deaths<<','<<m.captures<<','<<m.shots<<','<<m.deposits<<','<<m.clears<<','<<m.rooms<<','<<s.player.souls<<','<<s.depositedSouls<<','<<s.roomIndex<<','<<s.player.battery<<','<<std::hex<<digest(s)<<std::dec<<'\n';std::cout<<"SEED="<<seed<<" deaths="<<m.deaths<<" captures="<<m.captures<<" shots="<<m.shots<<" deposits="<<m.deposits<<" clears="<<m.clears<<" rooms="<<m.rooms<<" battery="<<s.player.battery<<"\n";} }
