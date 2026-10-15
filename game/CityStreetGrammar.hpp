#pragma once

#include "RoomEnvironment.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace city_street {

// One deterministic street section shared by rendering tiers. Quality changes
// optional repetition, never the road silhouette, curbs, intersection, or
// landmark rhythm that make the environment recognizable.
struct Plan {
    float roadHalfWidth=4.7f;
    float sidewalkWidth=2.1f;
    float curbHeight=0.11f;
    float intersectionCenter=4.0f;
    float intersectionWidth=5.2f;
    float lampInset=0.36f;
    int lampPairs=3;
    int serviceClusters=2;
    int skylineBands=5;
    unsigned char family=0;
};

inline Plan plan(room_environment::RoomForm form,std::uint32_t key,int graphicsPreset){
    Plan result;
    result.family=static_cast<unsigned char>(room_environment::mix(key+0x31u)%3u);
    switch(form){
        case room_environment::RoomForm::Canyon:
            result.roadHalfWidth=3.15f;result.sidewalkWidth=1.25f;result.intersectionWidth=4.3f;break;
        case room_environment::RoomForm::Courtyard:
            result.roadHalfWidth=5.35f;result.sidewalkWidth=2.45f;result.intersectionWidth=6.4f;break;
        case room_environment::RoomForm::Skyline:
            result.roadHalfWidth=6.55f;result.sidewalkWidth=2.15f;result.intersectionWidth=7.2f;break;
        default:
            result.roadHalfWidth=4.70f;result.sidewalkWidth=2.10f;result.intersectionWidth=5.2f;break;
    }
    result.intersectionCenter=-5.5f+room_environment::unit(key+0x47u)*11.0f;
    result.lampPairs=graphicsPreset<=0?2:(graphicsPreset==1?3:4);
    result.serviceClusters=graphicsPreset<=0?1:2;
    result.skylineBands=graphicsPreset<=0?3:(graphicsPreset==1?5:7);
    return result;
}

struct SkylineBlock {
    float x=0,z=0,width=1,height=1,depth=1;
    unsigned char family=0;
};

inline SkylineBlock skylineBlock(const Plan& street,std::uint32_t key,int tileIndex,int side,int index){
    const std::uint32_t local=room_environment::mix(key^static_cast<std::uint32_t>((tileIndex+17)*977+(side+2)*131+index*43));
    const float rank=static_cast<float>(index);
    SkylineBlock result;
    result.x=static_cast<float>(side)*(16.2f+rank*1.55f+room_environment::unit(local+1u)*1.2f);
    result.z=-17.0f+rank*(34.0f/static_cast<float>(std::max(1,street.skylineBands-1)))+(room_environment::unit(local+2u)-0.5f)*2.8f;
    result.width=2.4f+room_environment::unit(local+3u)*2.8f;
    result.height=5.8f+room_environment::unit(local+4u)*7.6f+(street.family==1?1.8f:0.0f);
    result.depth=3.2f+room_environment::unit(local+5u)*3.6f;
    result.family=static_cast<unsigned char>((street.family+index+side+3)%3);
    return result;
}

inline float lampZ(const Plan& street,int pair){
    return -15.5f+(31.0f/static_cast<float>(std::max(1,street.lampPairs-1)))*static_cast<float>(pair);
}

} // namespace city_street
