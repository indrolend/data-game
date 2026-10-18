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
    float captureActivity=0.0f;
    float completionAfterglow=0.0f;
};

struct Response {
    float shotLight=0.0f;
    float actionLight=0.0f;
    float criticalLight=0.0f;
    float phoneLightScale=1.0f;
    float exitGlow=0.0f;
    float captureLight=0.0f;
    float completionLight=0.0f;
};

inline Response resolve(const Inputs& input){
    Response response{};
    response.shotLight=1.0f-clampf(input.latestShotAge/0.18f,0.0f,1.0f);
    response.actionLight=clampf(input.discharge*0.82f+input.vacuumPower*0.18f,0.0f,1.0f);
    response.criticalLight=clampf(input.criticalPulse,0.0f,1.0f);
    response.phoneLightScale=1.0f+response.actionLight*0.28f+response.criticalLight*0.18f;
    response.exitGlow=clampf((input.roomClear?0.72f:0.0f)+clampf(input.objectiveProgress,0.0f,1.0f)*0.28f,0.0f,1.0f);
    const float capture=clampf(input.captureActivity,0.0f,1.0f);
    response.captureLight=capture*capture*(3.0f-2.0f*capture);
    response.phoneLightScale+=response.captureLight*0.72f;
    const float completion=clampf(input.completionAfterglow,0.0f,1.0f);
    response.completionLight=completion*completion;
    response.phoneLightScale+=response.completionLight*0.34f;
    return response;
}

} // namespace scene_lighting_response
