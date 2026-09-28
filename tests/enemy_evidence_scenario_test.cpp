#include "diagnostics/EnemyEvidenceScenario.hpp"

#include <cstdio>

int main() {
    struct Result { evidence::EnemyObstructionObservation observation{};bool finite=true,overlap=false,passed=false;int maximumStall=0; };
    const auto run=[](){
        Game game;evidence::configureEnemyObstruction(game);
        evidence::EnemyObstructionTracker tracker;tracker.reset(game);
        Result result;
        for(int tick=1;tick<=evidence::EnemyObstructionTicks;++tick){
            game.setTouchControls(0,0,0,0,false,false,false,false,false,false);
            game.update(1.0f/60.0f);
            result.observation=tracker.observe(game,tick);
            result.finite&=result.observation.finiteValues;
            result.overlap|=result.observation.colliderOverlap;
            result.passed|=result.observation.position.x>2.0f;
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
        first.maximumStall==second.maximumStall;
    const bool detoured=observation.maximumDetour>1.80f;
    const bool progressed=observation.progress>7.0f;
    if(!finite||overlap||!passed||!detoured||!progressed||maximumStall>90||!deterministic){
        std::fprintf(stderr,"ENEMY_EVIDENCE_SCENARIO_FAIL finite=%d overlap=%d passed=%d detour=%.3f progress=%.3f stall=%d deterministic=%d\n",
            finite?1:0,overlap?1:0,passed?1:0,observation.maximumDetour,observation.progress,maximumStall,deterministic?1:0);
        return 1;
    }
    std::printf("ENEMY_EVIDENCE_SCENARIO_OK detour=%.3f progress=%.3f stall=%d\n",observation.maximumDetour,observation.progress,maximumStall);
    return 0;
}
