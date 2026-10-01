#include "diagnostics/EnemyEvidenceScenario.hpp"

#include <cstdio>

int main() {
    struct Result { evidence::EnemyObstructionObservation observation{},firstOverlap{},maximumRecovery{};bool finite=true,overlap=false,passed=false,rememberedWhileOccluded=false,reacquired=false,observedSearch=false,observedEngage=false,observedSupport=false;int maximumStall=0,firstOverlapTick=0;float maximumRecoveryUrgency=0.0f,maximumBodyAttitude=0.0f,minimumMotorPace=2.0f,maximumMotorPace=0.0f; };
    const auto run=[](bool motorExpression){
        Game game;game.setEnemyMotorExpressionEnabled(motorExpression);evidence::configureEnemyObstruction(game);
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
            result.observedSupport|=result.observation.leftFootContact>0.8f||result.observation.rightFootContact>0.8f;
            if(result.observation.recoveryUrgency>result.maximumRecoveryUrgency){result.maximumRecoveryUrgency=result.observation.recoveryUrgency;result.maximumRecovery=result.observation;}
            result.maximumBodyAttitude=std::max(result.maximumBodyAttitude,
                std::max(std::abs(result.observation.bodyPitch),std::abs(result.observation.bodyRoll)));
            result.minimumMotorPace=std::min(result.minimumMotorPace,result.observation.motorPaceExpression);
            result.maximumMotorPace=std::max(result.maximumMotorPace,result.observation.motorPaceExpression);
            if(result.observation.goalDistance>2.0f)result.maximumStall=std::max(result.maximumStall,result.observation.stalledTicks);
        }
        return result;
    };
    const Result first=run(true),second=run(true),baseline=run(false);
    const auto& observation=first.observation;
    const bool finite=first.finite,overlap=first.overlap,passed=first.passed;
    const int maximumStall=first.maximumStall;
    const bool deterministic=std::abs(first.observation.position.x-second.observation.position.x)<0.000001f&&
        std::abs(first.observation.position.z-second.observation.position.z)<0.000001f&&
        std::abs(first.observation.maximumDetour-second.observation.maximumDetour)<0.000001f&&
        std::abs(first.observation.perceptionConfidence-second.observation.perceptionConfidence)<0.000001f&&
        first.maximumStall==second.maximumStall&&first.observation.cognition==second.observation.cognition;
    const bool detoured=observation.maximumDetour>1.80f&&observation.maximumDetour<4.0f;
    const bool progressed=observation.progress>7.0f;
    const bool embodied=first.observedSupport&&observation.footPlantChanges>=12&&
        observation.recoveryUrgency<0.30f&&first.maximumBodyAttitude<0.76f;
    const bool baselineValid=baseline.finite&&!baseline.overlap&&baseline.passed&&baseline.maximumStall<=90;
    const bool motorExpressive=first.maximumMotorPace-first.minimumMotorPace>0.01f&&
        (std::abs(first.observation.maximumDetour-baseline.observation.maximumDetour)>0.0001f||
         first.observation.footPlantChanges!=baseline.observation.footPlantChanges)&&
        observation.motorArousal>0.01f&&observation.motorFixation>0.01f;
    if(!finite||overlap||!passed||!detoured||!progressed||maximumStall>90||!deterministic||!first.rememberedWhileOccluded||!first.reacquired||!first.observedSearch||!first.observedEngage||!embodied||!baselineValid||!motorExpressive){
        std::fprintf(stderr,"ENEMY_EVIDENCE_SCENARIO_FAIL finite=%d overlap=%d overlap_tick=%d overlap_pos=%.3f,%.3f clearance=%.3f passed=%d detour=%.3f progress=%.3f stall=%d deterministic=%d memory=%d reacquired=%d search=%d engage=%d support=%d plants=%d recovery=%.3f@%d(%.3f,%.3f) final_recovery=%.3f posture=%.3f confidence=%.3f uncertainty=%.3f\n",
            finite?1:0,overlap?1:0,first.firstOverlapTick,first.firstOverlap.position.x,first.firstOverlap.position.z,first.firstOverlap.obstructionClearance,passed?1:0,observation.maximumDetour,observation.progress,maximumStall,deterministic?1:0,
            first.rememberedWhileOccluded?1:0,first.reacquired?1:0,first.observedSearch?1:0,first.observedEngage?1:0,
            first.observedSupport?1:0,observation.footPlantChanges,first.maximumRecoveryUrgency,first.maximumRecovery.tick,
            first.maximumRecovery.position.x,first.maximumRecovery.position.z,observation.recoveryUrgency,first.maximumBodyAttitude,
            observation.perceptionConfidence,observation.perceptionUncertainty);
        std::fprintf(stderr,"ENEMY_MOTOR_COMPARISON baseline_valid=%d expressive=%d pace=[%.3f,%.3f] arousal=%.3f caution=%.3f fixation=%.3f baseline_detour=%.3f motor_detour=%.3f baseline_plants=%d motor_plants=%d\n",
            baselineValid?1:0,motorExpressive?1:0,first.minimumMotorPace,first.maximumMotorPace,
            observation.motorArousal,observation.motorCaution,observation.motorFixation,
            baseline.observation.maximumDetour,observation.maximumDetour,
            baseline.observation.footPlantChanges,observation.footPlantChanges);
        return 1;
    }
    std::printf("ENEMY_EVIDENCE_SCENARIO_OK detour=%.3f progress=%.3f stall=%d memory=YES cognition=SEARCH_TO_ENGAGE plants=%d recovery=%.3f posture=%.3f reacquired=YES confidence=%.3f uncertainty=%.3f\n",
        observation.maximumDetour,observation.progress,maximumStall,observation.footPlantChanges,first.maximumRecoveryUrgency,first.maximumBodyAttitude,
        observation.perceptionConfidence,observation.perceptionUncertainty);
    std::printf("ENEMY_MOTOR_COMPARISON_OK pace=[%.3f,%.3f] arousal=%.3f caution=%.3f fixation=%.3f baseline_detour=%.3f motor_detour=%.3f baseline_plants=%d motor_plants=%d\n",
        first.minimumMotorPace,first.maximumMotorPace,observation.motorArousal,observation.motorCaution,observation.motorFixation,
        baseline.observation.maximumDetour,observation.maximumDetour,
        baseline.observation.footPlantChanges,observation.footPlantChanges);
    return 0;
}
