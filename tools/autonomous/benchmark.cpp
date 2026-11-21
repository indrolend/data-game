#define main old_audit_main
#include "audit_support.cpp"
#undef main
#include <chrono>
#include <limits>
static Action policy(int kind,int seed,int tick,const GameState& s,uint32_t& rng){
 if(kind==0) return {axis(rng),axis(rng),next(rng)%4==0,next(rng)%5==0,next(rng)%37==0,next(rng)%19==0,next(rng)%31==0};
 if(kind==1){ // fixed open-loop traversal/combat/vacuum sequence, independent of observations
  int phase=(tick/120)%5;return {0.f,phase==0?1.f:phase==1?-1.f:0.f,phase==2||phase==3,phase<2,tick%120==8,phase==2&&tick%15==0,phase==4&&tick%25==0};
 }
 // nearest-target feedback, using authoritative game state; no state mutation or debug cheats
 const TargetState* target=nullptr;float best=std::numeric_limits<float>::infinity();
 for(const auto& t:s.targets){if(!t.alive&&!t.slurpable)continue;float dx=t.pos.x-s.player.pos.x,dz=t.pos.z-s.player.pos.z,d=dx*dx+dz*dz;if(d<best){best=d;target=&t;}}
 if(!target)return {0,1,false,true,tick%150==8,false,false};
 float dx=target->pos.x-s.player.pos.x,dz=target->pos.z-s.player.pos.z;
 float length=std::sqrt(dx*dx+dz*dz);float x=length>0.01f?dx/length:0,z=length>0.01f?-dz/length:0;
 return {x,z,target->slurpable,true,tick%120==8,!target->slurpable&&length<2.5f&&tick%15==0,!target->slurpable&&length<8.f&&tick%25==0};
}
int main(int argc,char**argv){int seeds=argc>1?atoi(argv[1]):4;int ticks=argc>2?atoi(argv[2]):600;std::string dir=argc>3?argv[3]:"/mnt/data/autonomous/benchmark";if(seeds<1||seeds>64||ticks<1||ticks>3600)return 2;std::filesystem::create_directories(dir);std::ofstream out(dir+"/trials.csv");out<<"seed,policy,ticks,elapsed_ms,visited_buckets,enemy_deaths,objective_completions,room_transitions,stored_souls_max,first_violation,final_digest\n";int errors=0;auto start=std::chrono::steady_clock::now();for(int si=0;si<seeds;si++){int seed=12345+si*7919;for(int kind=0;kind<3;kind++){auto g=std::make_unique<Game>();g->reset();g->debugStartGeneratedRoomFixture(seed,2);uint32_t rng=uint32_t(seed)*2654435761u+uint32_t(2)*2246822519u+17;std::set<std::string> buckets;int deaths=0,completions=0,transitions=0,maxSouls=0,violation=-1;auto t0=std::chrono::steady_clock::now();for(int i=0;i<ticks;i++){const auto& s=g->state();Action a=policy(kind,seed,i,s,rng);int room=s.roomIndex;bool clear=s.roomClear;int alive=0;for(auto& t:s.targets)alive+=t.alive;apply(*g,a);const auto& c=g->state();int after=0;for(auto& t:c.targets)after+=t.alive;deaths+=std::max(0,alive-after);completions+=!clear&&c.roomClear;transitions+=room!=c.roomIndex;maxSouls=std::max(maxSouls,c.player.souls);if(violation<0&&!check(*g).empty())violation=i+1;std::ostringstream key;key<<c.roomIndex<<':'<<int(std::floor(c.player.pos.x/4))<<':'<<int(std::floor(c.player.pos.z/4))<<':'<<after<<':'<<c.roomClear;buckets.insert(key.str());}auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-t0).count();out<<seed<<','<<kind<<','<<ticks<<','<<elapsed<<','<<buckets.size()<<','<<deaths<<','<<completions<<','<<transitions<<','<<maxSouls<<','<<violation<<','<<std::hex<<digest(g->state())<<std::dec<<'\n';errors+=violation>=0;}}out.close();auto duration=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();printf("BENCHMARK seeds=%d ticks=%d trials=%d elapsed_ms=%lld invariant_failures=%d csv=%s\n",seeds,ticks,seeds*3,(long long)duration,errors,(dir+"/trials.csv").c_str());return errors?1:0;}
