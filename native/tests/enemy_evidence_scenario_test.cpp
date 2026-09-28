#include "diagnostics/EnemyEvidenceScenario.hpp"

#include <cstdio>

int main() {
    bool ok=true;
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
        ok=false;
    }
    std::printf("ENEMY_EVIDENCE_SCENARIO_OK detour=%.3f progress=%.3f stall=%d\n",observation.maximumDetour,observation.progress,maximumStall);

    const auto runContact=[](){
        Game game;evidence::configureEnemyContact(game);
        bool finite=true,started=false,hit=false,recovered=false;
        int hitTransitions=0;
        float previousBattery=game.state().player.battery,maxBatteryDrop=0.0f;
        bool previousHit=false;
        evidence::EnemyContactObservation observation{};
        for(int tick=1;tick<=evidence::EnemyContactTicks;++tick){
            game.setTouchControls(0,0,0,0,false,false,false,false,false,false);
            game.update(1.0f/60.0f);
            observation=evidence::observeEnemyContact(game,tick);
            observation.batteryDelta=observation.playerBattery-previousBattery;
            maxBatteryDrop=std::max(maxBatteryDrop,-observation.batteryDelta);previousBattery=observation.playerBattery;
            finite&=observation.finiteValues;
            started|=observation.attackActive;
            if(observation.attackHit&&!previousHit)++hitTransitions;
            hit|=observation.attackHit;
            recovered|=hit&&!observation.attackActive;
            previousHit=observation.attackHit;
        }
        struct ContactResult { evidence::EnemyContactObservation observation{};bool finite=false,started=false,hit=false,recovered=false;int hitTransitions=0;float maximumBatteryDrop=0.0f; };
        return ContactResult{observation,finite,started,hit,recovered,hitTransitions,maxBatteryDrop};
    };
    const auto contactFirst=runContact(),contactSecond=runContact();
    const bool contactDeterministic=std::abs(contactFirst.observation.playerBattery-contactSecond.observation.playerBattery)<0.000001f&&
        std::abs(contactFirst.observation.playerPosition.x-contactSecond.observation.playerPosition.x)<0.000001f&&
        std::abs(contactFirst.maximumBatteryDrop-contactSecond.maximumBatteryDrop)<0.000001f&&
        contactFirst.hitTransitions==contactSecond.hitTransitions;
    const bool contactPassed=contactFirst.finite&&contactFirst.started&&contactFirst.hit&&contactFirst.recovered&&
        contactFirst.hitTransitions==1&&contactFirst.maximumBatteryDrop>20.0f&&contactDeterministic;
    if(!contactPassed){
        std::fprintf(stderr,"ENEMY_CONTACT_SCENARIO_FAIL finite=%d started=%d hit=%d recovered=%d hit_transitions=%d maximum_battery_drop=%.3f deterministic=%d\n",
            contactFirst.finite?1:0,contactFirst.started?1:0,contactFirst.hit?1:0,contactFirst.recovered?1:0,
            contactFirst.hitTransitions,contactFirst.maximumBatteryDrop,contactDeterministic?1:0);
        ok=false;
    } else std::printf("ENEMY_CONTACT_SCENARIO_OK hit_transitions=%d maximum_battery_drop=%.3f\n",contactFirst.hitTransitions,contactFirst.maximumBatteryDrop);
    return ok?0:1;
}
