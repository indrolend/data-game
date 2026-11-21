// Standalone Zeptoverse trial: compare a scalar/unrolled AABB segment kernel
// against the shipping implementation. Does not change gameplay authority.
// Build (from repo root):
//   c++ -std=c++17 -O2 -Igame experiments/zeptoverse/segment_box_trial.cpp -o segment_box_trial
// Run: ./segment_box_trial
#include "gameplay/EnemyPerception.hpp"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

struct Case { Vec3 a,b,lo,hi; };
static uint32_t seed=0x12345678u;
static uint32_t rnd(){ seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed; }
static float value(){ return float(int(rnd()%20001)-10000)*0.001f; }
static bool axis(float origin,float direction,float minimum,float maximum,float& nearT,float& farT){
    if(std::abs(direction)<0.00001f)
        return !(origin<minimum||origin>maximum);
    float lo=(minimum-origin)/direction,hi=(maximum-origin)/direction;
    if(lo>hi){const float tmp=lo;lo=hi;hi=tmp;}
    nearT=std::max(nearT,lo);
    farT=std::min(farT,hi);
    return nearT<=farT;
}
static bool candidate(const Vec3& start,const Vec3& end,const Vec3& minimum,const Vec3& maximum,float& entry){
    const Vec3 a=gameplay::finitePerceptionVector(start);
    const Vec3 b=gameplay::finitePerceptionVector(end);
    const Vec3 d=b-a;
    float nearT=0.0f,farT=1.0f;
    if(!axis(a.x,d.x,minimum.x,maximum.x,nearT,farT))return false;
    if(!axis(a.y,d.y,minimum.y,maximum.y,nearT,farT))return false;
    if(!axis(a.z,d.z,minimum.z,maximum.z,nearT,farT))return false;
    entry=nearT;
    return farT>=0.0f&&nearT<=1.0f;
}
static bool sameBits(float a,float b){return std::memcmp(&a,&b,sizeof(float))==0;}
int main(){
    std::vector<Case> cases;
    cases.reserve(200008);
    for(int i=0;i<200000;++i){
        Case c{{value(),value(),value()},{value(),value(),value()},
               {value(),value(),value()},{value(),value(),value()}};
        if(i%3==0){c.hi={c.lo.x+std::abs(value()),c.lo.y+std::abs(value()),c.lo.z+std::abs(value())};}
        if(i%7==0)c.b.x=c.a.x;
        if(i%11==0)c.b.y=c.a.y;
        if(i%13==0)c.b.z=c.a.z;
        cases.push_back(c);
    }
    cases.push_back({{0,1,0},{0,1,-8},{-.3f,0,-4.3f},{.3f,3,-3.7f}});
    cases.push_back({{0,0,0},{0,0,0},{-1,-1,-1},{1,1,1}});
    cases.push_back({{2,2,2},{2,2,2},{-1,-1,-1},{1,1,1}});
    cases.push_back({{0,0,0},{0,0,0},{0,0,0},{0,0,0}});
    cases.push_back({{0,0,0},{1,0,0},{1,0,0},{2,1,1}});
    cases.push_back({{0,0,0},{1,0,0},{-1,-1,-1},{0,1,1}});
    cases.push_back({{0,0,0},{1,0,0},{-1,-1,-1},{2,1,1}});
    cases.push_back({{0,0,0},{1,0,0},{2,-1,-1},{3,1,1}});
    unsigned mismatches=0;
    for(const Case& c:cases){
        float a=123.0f,b=123.0f;
        bool x=gameplay::perceptionSegmentHitsBox(c.a,c.b,c.lo,c.hi,a);
        bool y=candidate(c.a,c.b,c.lo,c.hi,b);
        if(x!=y||!sameBits(a,b)){if(++mismatches<=4)std::printf("MISMATCH a=%d b=%d entry=%g/%g\n",x,y,a,b);}
    }
    volatile uint64_t sink=0;
    auto measure=[&](bool alternate){
        const auto t0=std::chrono::steady_clock::now();
        for(int repeat=0;repeat<20;++repeat)for(const Case& c:cases){
            float entry=0;
            bool hit=alternate?candidate(c.a,c.b,c.lo,c.hi,entry):
                gameplay::perceptionSegmentHitsBox(c.a,c.b,c.lo,c.hi,entry);
            sink+=uint64_t(hit);
        }
        return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
    };
    const double baseline=measure(false),alternative=measure(true);
    std::printf("CASES=%zu MISMATCHES=%u BASELINE_MS=%.3f CANDIDATE_MS=%.3f SINK=%llu\n",
        cases.size(),mismatches,baseline,alternative,(unsigned long long)sink);
    return mismatches?1:0;
}
