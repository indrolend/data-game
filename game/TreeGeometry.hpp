#pragma once

#include "RoomEnvironment.hpp"

#include <array>
#include <cmath>

namespace tree_geometry {

struct Part { Vec3 center{};Vec3 size{1,1,1};float yaw=0; };
enum class Archetype : unsigned char { Sentinel, Broad, WindShaped };
struct Profile {
    Archetype archetype=Archetype::Sentinel;
    float trunkHeight=0.80f;
    float baseRadius=0.145f;
    float crownWidth=1.0f;
    float crownHeight=1.0f;
    float leanX=0.0f;
    float leanZ=0.0f;
    int branchCount=4;
};
inline Profile profile(const room_environment::EnvironmentPropSpec& prop);

// Runtime visual variation derived from one authored family. Collider size and
// climb authority remain on EnvironmentPropSpec; these parameters only change
// growth history and silhouette, never gameplay reachability.
struct AuthoredVariant {
    float widthScale=1.0f;
    float depthScale=1.0f;
    float pitch=0.0f;
    float roll=0.0f;
    float yawJitter=0.0f;
    float rootSpread=1.0f;
};

inline AuthoredVariant authoredVariant(const room_environment::EnvironmentPropSpec& prop,unsigned int key){
    const auto profile_=profile(prop);
    AuthoredVariant result;
    result.widthScale=0.90f+room_environment::unit(key+11u)*0.22f;
    result.depthScale=0.88f+room_environment::unit(key+17u)*0.25f;
    result.pitch=(room_environment::unit(key+23u)-0.5f)*0.075f;
    result.roll=(room_environment::unit(key+29u)-0.5f)*0.085f;
    if(profile_.archetype==Archetype::WindShaped)result.roll+=prop.yaw<0.0f?-0.10f:0.10f;
    result.yawJitter=(room_environment::unit(key+31u)-0.5f)*0.34f;
    result.rootSpread=0.82f+room_environment::unit(key+37u)*0.42f;
    return result;
}

inline Profile profile(const room_environment::EnvironmentPropSpec& prop){
    switch(prop.variant%3u){
        case 1:return {Archetype::Broad,0.72f,0.175f,1.16f,0.88f,-0.025f,0.018f,6};
        case 2:return {Archetype::WindShaped,0.78f,0.135f,0.92f,0.94f,0.13f,-0.055f,5};
        default:return {Archetype::Sentinel,0.84f,0.125f,0.82f,1.10f,0.018f,0.012f,4};
    }
}
constexpr int TrunkPartCount=3;
constexpr int CrownPartCount=7;

inline std::array<Part,TrunkPartCount> trunkParts(const room_environment::EnvironmentPropSpec& prop,float zOffset=0.0f){
    const Vec3 origin=prop.center+Vec3{0,0,zOffset};
    return {{
        {origin+Vec3{-prop.size.x*0.018f,prop.size.y*0.18f,0},{prop.size.x*0.26f,prop.size.y*0.36f,prop.size.z*0.26f},prop.yaw-0.035f},
        {origin+Vec3{ prop.size.x*0.012f,prop.size.y*0.49f,0},{prop.size.x*0.21f,prop.size.y*0.30f,prop.size.z*0.21f},prop.yaw+0.025f},
        {origin+Vec3{-prop.size.x*0.010f,prop.size.y*0.70f,0},{prop.size.x*0.16f,prop.size.y*0.18f,prop.size.z*0.16f},prop.yaw-0.018f}
    }};
}

inline std::array<Part,CrownPartCount> crownParts(const room_environment::EnvironmentPropSpec& prop,float zOffset=0.0f){
    std::array<Part,CrownPartCount> result{};const Vec3 origin=prop.center+Vec3{0,0,zOffset};
    const float c=std::cos(prop.yaw),s=std::sin(prop.yaw);int index=0;
    for(const auto& cluster:room_environment::treeFoliageClusters()){
        const Vec3 local{cluster.offset.x*prop.size.x,cluster.offset.y*prop.size.y,cluster.offset.z*prop.size.z};
        result[index++]={origin+Vec3{local.x*c+local.z*s,local.y,-local.x*s+local.z*c},{cluster.scale.x*prop.size.x,cluster.scale.y*prop.size.y,cluster.scale.z*prop.size.z},prop.yaw+cluster.yawOffset};
    }
    return result;
}

inline float climbTopY(const room_environment::EnvironmentPropSpec& prop){return prop.center.y+prop.size.y*1.18f;}

} // namespace tree_geometry
