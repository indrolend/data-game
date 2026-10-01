#include "diagnostics/EnemyEvidenceScenario.hpp"

#include <cstdio>

int main() {
    struct Result { evidence::EnemyObstructionObservation observation{},firstOverlap{};bool finite=true,overlap=false,passed=false,rememberedWhileOccluded=false,reacquired=false,observedSearch=false,observedEngage=false;int maximumStall=0,firstOverlapTick=0; };
    const auto run=[](){
        Game game;evidence::configureEnemyObstruction(game);
        evidence::EnemyObstructionTracker tracker;tracker.reset(game);
        Result result;
        for(int tick=1;tick<=evidence::EnemyObstructionTicks;++tick){
            game.setTouchControls(0,0,0,0,false,false,false,false,false,false);
            game.update(1.0f/60.0f);
            result.observation=tracker.observe(game,tick);
            result.finite&=result.observation.finiteValues;
            if(result.observation.colliderOverlap&&!result.overlap){result.firstOverlapTick=tick;result.firstOverlap=result.observation;}
            result.overlap|=result.observation.colliderOverlap;
            result.passed|=result.observation.position.x>2.0f;
            result.rememberedWhileOccluded|=!result.observation.perceptionConfirmed&&result.observation.hasSpatialBelief;
            result.reacquired|=result.passed&&result.observation.perceptionConfirmed;
            result.observedSearch|=result.observation.cognition==gameplay::EnemyBehaviorMode::Search;
            result.observedEngage|=result.observation.cognition==gameplay::EnemyBehaviorMode::Engage;
            if(result.observation.goalDistance>2.0f)result.maximumStall=std::max(result.maximumStall,result.observation.stalledTicks);
        }
        return result;
    };
    const Result first=run(),second=run();
    const auto& observation=first.observation;
    const bool finite=first.finite,overlap=first.overlap,passed=first.passed;
    const int maximumStall=first.maximumStall;
    const bool deterministic=std::abs(first.observation.position.x-second.observation.position.x)<0.000001f&&
        std::abs(first.observation.position.z-second.observation.position.z)<0.000001f&&
        std::abs(first.observation.maximumDetour-second.observation.maximumDetour)<0.000001f&&
        std::abs(first.observation.perceptionConfidence-second.observation.perceptionConfidence)<0.000001f&&
        first.maximumStall==second.maximumStall&&first.observation.cognition==second.observation.cognition;
    const bool detoured=observation.maximumDetour>1.80f;
    const bool progressed=observation.progress>7.0f;
    if(!finite||overlap||!passed||!detoured||!progressed||maximumStall>90||!deterministic||!first.rememberedWhileOccluded||!first.reacquired||!first.observedSearch||!first.observedEngage){
        std::fprintf(stderr,"ENEMY_EVIDENCE_SCENARIO_FAIL finite=%d overlap=%d overlap_tick=%d overlap_pos=%.3f,%.3f clearance=%.3f passed=%d detour=%.3f progress=%.3f stall=%d deterministic=%d memory=%d reacquired=%d confidence=%.3f uncertainty=%.3f\n",
            finite?1:0,overlap?1:0,first.firstOverlapTick,first.firstOverlap.position.x,first.firstOverlap.position.z,first.firstOverlap.obstructionClearance,passed?1:0,observation.maximumDetour,observation.progress,maximumStall,deterministic?1:0,
            first.rememberedWhileOccluded?1:0,first.reacquired?1:0,observation.perceptionConfidence,observation.perceptionUncertainty);
        return 1;
    }
    std::printf("ENEMY_EVIDENCE_SCENARIO_OK detour=%.3f progress=%.3f stall=%d memory=YES cognition=SEARCH_TO_ENGAGE reacquired=YES confidence=%.3f uncertainty=%.3f\n",
        observation.maximumDetour,observation.progress,maximumStall,observation.perceptionConfidence,observation.perceptionUncertainty);
    return 0;
}
