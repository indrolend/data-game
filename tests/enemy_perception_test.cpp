#include "gameplay/EnemyPerception.hpp"
#include "FacetedRock.hpp"

#include <cassert>
#include <cmath>
#include <limits>

namespace {
constexpr float Dt=1.0f/60.0f;
bool finiteVec(const Vec3& v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
gameplay::EnemyPerceptionOutput observe(gameplay::EnemyPerceptionState& state,const Vec3& target,float visibility,const Vec3& velocity={},float bodyYaw=0.0f,bool sampled=true){
    gameplay::EnemyPerceptionInput input{};input.observerPosition={0,1.65f,0};input.targetPosition=target;input.targetVelocity=velocity;
    input.bodyYaw=bodyYaw;input.visibility=visibility;input.sampled=sampled;input.dt=Dt;input.individuality=0.17f;
    return gameplay::updateEnemyPerception(input,state);
}
}

int main(){
    gameplay::EnemyPerceptionState clear{},blocked{};
    const auto seen=observe(clear,{0,1,-6},0.75f),unseen=observe(blocked,{0,1,-6},0.0f);
    assert(seen.confirmed&&seen.hasSpatialBelief&&!unseen.confirmed&&!unseen.hasSpatialBelief);
    assert(gameplay::visualAcquisitionStrength(5,1,0,0)==0.0f);
    assert(gameplay::visualAcquisitionStrength(5,1,0,1)>gameplay::visualAcquisitionStrength(5,0,0,1));
    assert(gameplay::visualAcquisitionStrength(5,0,5,1)>gameplay::visualAcquisitionStrength(5,0,0,1));

    float entry=0.0f;
    assert(gameplay::perceptionSegmentHitsBox({0,1,0},{0,1,-8},{-0.3f,0,-4.3f},{0.3f,3,-3.7f},entry));
    using namespace room_environment;
    const EnvironmentPropSpec rock{EnvironmentPrimitive::Rock,EnvironmentRole::Landmark,{0,0,-4},{2,2,1.4f},0,0};
    const auto mesh=faceted_rock::makeMesh(rock,91,7,2);const auto bounds=faceted_rock::meshBounds(mesh);
    const float upperY=bounds.maximum.y-0.01f;
    assert(upperY>rock.center.y+rock.size.y*0.5f);
    assert(gameplay::perceptionSegmentHitsBox({-3,upperY,-4},{3,upperY,-4},bounds.minimum,bounds.maximum,entry));

    const Vec3 remembered=seen.believedPosition;float initialConfidence=seen.confidence,initialUncertainty=seen.uncertainty;
    gameplay::EnemyPerceptionOutput lost{};float minYaw=1.18f,maxYaw=-1.18f;
    for(int frame=0;frame<90;++frame){lost=observe(clear,{20,1,15},0,{9,0,0},0,frame==0);minYaw=std::min(minYaw,lost.headYaw);maxYaw=std::max(maxYaw,lost.headYaw);}
    assert(lost.hasSpatialBelief&&!lost.confirmed&&lost.believedPosition.x==remembered.x&&lost.believedPosition.z==remembered.z);
    assert(lost.confidence<initialConfidence&&lost.uncertainty>initialUncertainty&&maxYaw-minYaw>0.03f);
    const auto reacquired=observe(clear,{-2,1,-4},0.9f);assert(reacquired.confirmed&&reacquired.believedPosition.x==-2);

    gameplay::EnemyPerceptionState aware{};gameplay::EnemyPerceptionInput awareness{};
    awareness.observerPosition={0,1,0};awareness.targetPosition={99,1,99};awareness.vagueAwareness=1;awareness.dt=Dt;
    for(int frame=0;frame<120;++frame)gameplay::updateEnemyPerception(awareness,aware);
    assert(aware.confidence==0&&aware.lastSeenPosition.x==0&&std::abs(aware.headYaw)>0.01f);

    gameplay::EnemyPerceptionState poison{};poison.lastSeenPosition={NAN,INFINITY,-INFINITY};poison.lastSeenVelocity=poison.lastSeenPosition;
    poison.confidence=NAN;poison.uncertainty=INFINITY;poison.headYaw=NAN;poison.headPitch=INFINITY;poison.searchPhase=NAN;
    gameplay::EnemyPerceptionInput bad{};bad.observerPosition=poison.lastSeenPosition;bad.targetPosition=poison.lastSeenPosition;
    bad.targetVelocity=poison.lastSeenPosition;bad.bodyYaw=NAN;bad.visibility=INFINITY;bad.dt=INFINITY;bad.sampled=true;
    const auto safe=gameplay::updateEnemyPerception(bad,poison);
    assert(finiteVec(safe.believedPosition)&&finiteVec(safe.headDirection)&&std::isfinite(safe.confidence)&&std::isfinite(safe.uncertainty));

    gameplay::EnemyPerceptionState first{},second{};
    for(int frame=0;frame<600;++frame){const bool sample=frame%13==0;const float v=(frame/90)%2?0.0f:0.64f;const Vec3 p{std::sin(frame*0.03f)*4,1,-5};const auto a=observe(first,p,v,{1,0,0},0.2f,sample),b=observe(second,p,v,{1,0,0},0.2f,sample);assert(a.headYaw==b.headYaw&&a.confidence==b.confidence&&a.believedPosition.x==b.believedPosition.x);}
    return 0;
}
