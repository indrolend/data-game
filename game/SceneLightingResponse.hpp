#pragma once

#include "Math.hpp"

namespace scene_lighting_response {

struct Inputs {
    float vacuumPower=0.0f;
    float discharge=0.0f;
    float latestShotAge=9999.0f;
    float criticalPulse=0.0f;
    float objectiveProgress=0.0f;
    bool roomClear=false;
};

struct Response {
    float shotLight=0.0f;
    float actionLight=0.0f;
    float criticalLight=0.0f;
    float phoneLightScale=1.0f;
    float exitGlow=0.0f;
};

inline Response resolve(const Inputs& input){
    Response response{};
    response.shotLight=1.0f-clampf(input.latestShotAge/0.18f,0.0f,1.0f);
    response.actionLight=clampf(input.discharge*0.82f+input.vacuumPower*0.18f,0.0f,1.0f);
    response.criticalLight=clampf(input.criticalPulse,0.0f,1.0f);
    response.phoneLightScale=1.0f+response.actionLight*0.28f+response.criticalLight*0.18f;
    response.exitGlow=clampf((input.roomClear?0.72f:0.0f)+clampf(input.objectiveProgress,0.0f,1.0f)*0.28f,0.0f,1.0f);
    return response;
}

} // namespace scene_lighting_response
