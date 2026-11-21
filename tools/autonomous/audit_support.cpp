#include "Game.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <filesystem>
struct Action {float x,z;bool vacuum,sprint,jump,melee,shoot;};
static uint32_t next(uint32_t& s){s^=s<<13;s^=s>>17;s^=s<<5;return s;}
static float axis(uint32_t&s){return float(int(next(s)%2001)-1000)/1000.f;}
static std::vector<Action> actions(int seed,int room,int n){uint32_t r=uint32_t(seed)*2654435761u+uint32_t(room)*2246822519u+17;std::vector<Action>a;for(int i=0;i<n;i++){if(i%30==0){/* independent actions per interval, deterministic seed */}a.push_back({axis(r),axis(r),next(r)%4==0,next(r)%5==0,next(r)%37==0,next(r)%19==0,next(r)%31==0});}return a;}
static void apply(Game&g,const Action&a){g.setTouchControls(a.x,a.z,0,0,a.vacuum,a.sprint,a.jump,a.melee,a.shoot,false);g.update(1.f/60.f);}
static uint64_t mix(uint64_t h,uint32_t v){for(int j=0;j<4;j++){h^=(v>>(j*8))&255;h*=1099511628211ULL;}return h;}
static uint32_t bits(float f){uint32_t v;std::memcpy(&v,&f,4);return v;}
static uint64_t digest(const GameState&s){uint64_t h=14695981039346656037ull;auto f=[&](float v){h=mix(h,bits(v));};h=mix(h,s.frame);h=mix(h,s.roomIndex);f(s.time);for(float v:{s.player.pos.x,s.player.pos.y,s.player.pos.z,s.player.vel.x,s.player.vel.y,s.player.vel.z,s.player.battery,s.camera.pos.x,s.camera.pos.y,s.camera.pos.z,s.camera.yaw,s.camera.pitch})f(v);h=mix(h,s.player.souls);h=mix(h,s.depositedSouls);h=mix(h,s.roomClear);h=mix(h,s.dead);for(auto&t:s.targets){h=mix(h,t.alive);f(t.pos.x);f(t.pos.y);f(t.pos.z);f(t.health);}return h;}
static std::string check(const Game&g){const auto&s=g.state();auto finite=[](float f){return std::isfinite(f);};for(float f:{s.player.pos.x,s.player.pos.y,s.player.pos.z,s.player.vel.x,s.player.vel.y,s.player.vel.z,s.player.battery,s.camera.pos.x,s.camera.pos.y,s.camera.pos.z,s.camera.yaw,s.camera.pitch,s.time})if(!finite(f))return "nonfinite_core_state";if(s.player.battery<-.01f||s.player.battery>100.01f)return "battery_out_of_range";if(s.player.souls<0||s.player.souls>PHONE_CAPACITY)return "souls_out_of_range";if(s.depositedSouls<0||s.depositedSouls>s.requiredSouls)return "objective_out_of_range";if(s.player.grounded){auto support=g.debugPlayerSupportAt(s.player.pos.x,s.player.pos.z);if(support.available&&std::abs(s.player.pos.y-support.height)>0.35f)return "grounded_support_disagreement";}return "";}
struct Outcome{std::vector<uint64_t> hashes;int firstFailure=-1;std::string failure;float distance=0,battery=0;int room=0;std::set<std::string> visited;int roomTransitions=0,enemyDeaths=0,objectiveCompletions=0;};
static Outcome run(int seed,int room,const std::vector<Action>&a,int count){auto g=std::make_unique<Game>();g->reset();g->debugStartGeneratedRoomFixture(seed,room);Outcome o;o.hashes.reserve(count+1);o.hashes.push_back(digest(g->state()));
 auto observe=[&](){const auto&s=g->state();int alive=0;for(const auto&t:s.targets)alive+=t.alive?1:0;
 std::ostringstream key;key<<s.roomIndex<<":"<<int(std::floor(s.player.pos.x/4))<<":"<<int(std::floor(s.player.pos.z/4))<<":"<<alive<<":"<<s.roomClear;
 o.visited.insert(key.str());};observe();
 for(int i=0;i<count;i++){auto prev=g->state();apply(*g,a[i]);const auto&cur=g->state();
 if(cur.roomIndex!=prev.roomIndex)o.roomTransitions++;
 if(!prev.roomClear&&cur.roomClear)o.objectiveCompletions++;
 for(size_t j=0;j<std::min(prev.targets.size(),cur.targets.size());j++)if(prev.targets[j].alive&&!cur.targets[j].alive)o.enemyDeaths++;
 observe();auto err=check(*g);if(!err.empty()&&o.firstFailure<0){o.firstFailure=i+1;o.failure=err;}o.hashes.push_back(digest(g->state()));}const auto&s=g->state();o.distance=std::hypot(s.player.pos.x,s.player.pos.z-15.5f);o.battery=s.player.battery;o.room=s.roomIndex;return o;}
static bool negativeControls(){auto g=std::make_unique<Game>();g->reset();g->debugStartGeneratedRoomFixture(12345,1);auto baseline=check(*g);g->hostMutableState().player.battery=150.f;auto invalid=check(*g);g->hostMutableState().player.battery=100.f;auto a=actions(12345,1,120);auto b=a;b[10].x=1.f;b[10].z=-1.f;auto original=run(12345,1,a,120),mutated=run(12345,1,b,120);bool divergent=false;for(size_t i=0;i<original.hashes.size();i++)if(original.hashes[i]!=mutated.hashes[i]){divergent=true;break;}printf("NEGATIVE_CONTROL battery_detected=%d input_mutation_detected=%d\n",invalid=="battery_out_of_range",divergent);return baseline.empty()&&invalid=="battery_out_of_range"&&divergent;}
int main(int argc,char**argv){int n=argc>1?atoi(argv[1]):360;std::string dir=argc>2?argv[2]:".";if(n<1||n>10000)return 2;if(!negativeControls())return 3;std::ofstream csv(dir+"/trials.csv"),fail(dir+"/failures.jsonl"),spec(dir+"/specification.json");csv<<"seed,room,ticks,first_violation,violation,replay_mismatches,final_room,battery,planar_displacement,final_digest,visited_state_buckets,room_transitions,enemy_deaths,objective_completions\n";int trials=0,violations=0,mismatches=0;std::set<std::string> totalBuckets;int transitions=0,deaths=0,completions=0;for(int seed:{12345,45678,98765,314159,271828,42,1001,9001})for(int room:{1,2,3}){auto a=actions(seed,room,n);auto x=run(seed,room,a,n),y=run(seed,room,a,n);int mismatch=0;for(int i=0;i<=n;i++)if(x.hashes[i]!=y.hashes[i])mismatch++;trials++;mismatches+=mismatch;totalBuckets.insert(x.visited.begin(),x.visited.end());transitions+=x.roomTransitions;deaths+=x.enemyDeaths;completions+=x.objectiveCompletions;if(x.firstFailure>=0){violations++;int lo=0,hi=x.firstFailure;while(lo+1<hi){int mid=(lo+hi)/2;auto p=run(seed,room,a,mid);if(p.firstFailure>=0)hi=mid;else lo=mid;}fail<<"{\"seed\":"<<seed<<",\"room\":"<<room<<",\"first_failure_tick\":"<<hi<<",\"type\":\""<<x.failure<<"\"}\n";}csv<<seed<<','<<room<<','<<n<<','<<x.firstFailure<<','<<x.failure<<','<<mismatch<<','<<x.room<<','<<x.battery<<','<<x.distance<<','<<std::hex<<x.hashes.back()<<std::dec<<','<<x.visited.size()<<','<<x.roomTransitions<<','<<x.enemyDeaths<<','<<x.objectiveCompletions<<'\n';printf("TRIAL seed=%d room=%d violations=%d replay_mismatches=%d final_room=%d\n",seed,room,x.firstFailure>=0,mismatch,x.room);}
spec<<"{\"schema\":1,\"simulation_step_seconds\":0.016666666666666666,\"trials\":"<<trials<<",\"ticks_per_trial\":"<<n<<",\"checked_invariants\":[\"finite_core_state\",\"battery_in_0_100\",\"souls_in_capacity\",\"deposited_within_required\",\"grounded_player_near_support\"],\"determinism_scope\":\"selected_authoritative_state_fields\",\"failures\":"<<violations<<",\"replay_mismatches\":"<<mismatches<<"}\n";printf("SUMMARY trials=%d ticks=%d invariant_violations=%d replay_mismatches=%d state_buckets=%zu room_transitions=%d enemy_deaths=%d objective_completions=%d\n",trials,trials*n,violations,mismatches,totalBuckets.size(),transitions,deaths,completions);return violations||mismatches?1:0;}
