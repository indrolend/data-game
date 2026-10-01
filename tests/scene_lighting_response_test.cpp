#include "SceneLightingResponse.hpp"

#include <cmath>
#include <cstdio>

int main(){
    using namespace scene_lighting_response;
    const auto calm=resolve({0.0f,0.0f,9999.0f,0.0f,0.0f,false});
    const auto shot=resolve({0.0f,0.0f,0.0f,0.0f,0.0f,false});
    const auto active=resolve({1.0f,1.0f,9999.0f,1.0f,0.5f,false});
    const auto clear=resolve({0.0f,0.0f,9999.0f,0.0f,1.0f,true});
    const auto expiredShot=resolve({0.0f,0.0f,0.18f,0.0f,0.0f,false});
    if(calm.shotLight!=0.0f||calm.actionLight!=0.0f||calm.criticalLight!=0.0f||calm.phoneLightScale!=1.0f||calm.exitGlow!=0.0f||
        shot.shotLight!=1.0f||expiredShot.shotLight!=0.0f||active.actionLight!=1.0f||active.criticalLight!=1.0f||active.phoneLightScale<=1.0f||
        active.exitGlow<=0.0f||active.exitGlow>=clear.exitGlow||clear.exitGlow!=1.0f){
        std::fputs("SCENE_LIGHTING_RESPONSE_FAIL bounded response\n",stderr);return 1;
    }
    const auto adversarial=resolve({INFINITY,-INFINITY,NAN,INFINITY,-INFINITY,false});
    if(!std::isfinite(adversarial.shotLight)||!std::isfinite(adversarial.actionLight)||!std::isfinite(adversarial.criticalLight)||!std::isfinite(adversarial.phoneLightScale)||!std::isfinite(adversarial.exitGlow)){
        std::fputs("SCENE_LIGHTING_RESPONSE_FAIL finite recovery\n",stderr);return 1;
    }
    std::puts("SCENE_LIGHTING_RESPONSE_OK shot=BOUNDED action=BOUNDED critical=BOUNDED exit=OBJECTIVE phone=DERIVED");
    return 0;
}
