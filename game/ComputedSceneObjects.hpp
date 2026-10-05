#pragma once

#include "HouseGeometry.hpp"
#include "MarkerPillarGeometry.hpp"
#include "RoomEnvironment.hpp"
#include "RuinGeometry.hpp"
#include "TreeGeometry.hpp"
#include "world/RoomGeometry.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace computed_scene {

enum class GeometryKind : unsigned char { Box, FacetedMesh };
enum class PartKind : unsigned char { Whole, House, TreeTrunk, TreeCrown, Ruin, MarkerPillar };

struct Object {
    std::string id;
    int tileIndex=0;
    int propIndex=-1;
    int partIndex=0;
    room_environment::EnvironmentPrimitive primitive=room_environment::EnvironmentPrimitive::MarkerPillar;
    room_environment::EnvironmentRole role=room_environment::EnvironmentRole::Detail;
    GeometryKind geometry=GeometryKind::Box;
    PartKind part=PartKind::Whole;
    Vec3 center{};
    Vec3 size{1,1,1};
    float yaw=0;
    unsigned char surface=0;
    bool physical=false;
};

struct IdColor { unsigned char r=0,g=0,b=0; };

constexpr IdColor idColor(std::uint32_t oneBasedId){
    return {static_cast<unsigned char>(oneBasedId&0xffu),static_cast<unsigned char>((oneBasedId>>8)&0xffu),static_cast<unsigned char>((oneBasedId>>16)&0xffu)};
}
constexpr std::uint32_t idFromColor(IdColor color){return static_cast<std::uint32_t>(color.r)|(static_cast<std::uint32_t>(color.g)<<8)|(static_cast<std::uint32_t>(color.b)<<16);}

inline const char* primitiveName(room_environment::EnvironmentPrimitive primitive){
    using P=room_environment::EnvironmentPrimitive;
    switch(primitive){case P::House:return "house";case P::Tree:return "tree";case P::LawnFragment:return "lawn";case P::MarkerPillar:return "marker-pillar";case P::Ruin:return "ruin";case P::Rock:return "rock";}return "unknown";
}
inline const char* partName(PartKind part){
    switch(part){case PartKind::Whole:return "whole";case PartKind::House:return "house";case PartKind::TreeTrunk:return "trunk";case PartKind::TreeCrown:return "crown";case PartKind::Ruin:return "ruin";case PartKind::MarkerPillar:return "pillar";}return "unknown";
}

inline std::string objectId(int tileIndex,int propIndex,room_environment::EnvironmentPrimitive primitive,PartKind part,int partIndex){
    return "room/"+std::to_string(tileIndex)+"/prop/"+std::to_string(propIndex)+"/"+primitiveName(primitive)+"/"+partName(part)+"/"+std::to_string(partIndex);
}

inline std::vector<Object> environmentObjects(const room_environment::RoomEnvironmentPlan& plan,int roomSeed,int roomIndex,int tileIndex,const room_environment::RoomGeometryCapacityPlan& capacity){
    using room_environment::EnvironmentPrimitive;
    std::vector<Object> result;
    const float zOffset=static_cast<float>(tileIndex)*world::RoomDepth;
    const int count=room_environment::environmentPropCount(plan);
    const auto append=[&](int propIndex,const auto& prop,PartKind kind,int partIndex,const Vec3& center,const Vec3& size,float yaw,unsigned char surface,bool physical,GeometryKind geometry=GeometryKind::Box){
        result.push_back({objectId(tileIndex,propIndex,prop.primitive,kind,partIndex),tileIndex,propIndex,partIndex,prop.primitive,prop.role,geometry,kind,center,size,yaw,surface,physical});
    };
    for(int propIndex=0;propIndex<count;++propIndex){
        if(!capacity.propIncluded[propIndex])continue;
        const auto prop=room_environment::environmentProp(plan,roomSeed,roomIndex,propIndex);
        if(prop.primitive==EnvironmentPrimitive::House){int partIndex=0;for(const auto& part:house_geometry::parts(prop,zOffset)){append(propIndex,prop,PartKind::House,partIndex++,part.center,part.size,part.yaw,part.surface,part.physical);}}
        else if(prop.primitive==EnvironmentPrimitive::Tree){int partIndex=0;for(const auto& part:tree_geometry::trunkParts(prop,zOffset)){append(propIndex,prop,PartKind::TreeTrunk,partIndex++,part.center,part.size,part.yaw,0,true);}partIndex=0;for(const auto& part:tree_geometry::crownParts(prop,zOffset)){append(propIndex,prop,PartKind::TreeCrown,partIndex++,part.center,part.size,part.yaw,1,false);}}
        else if(prop.primitive==EnvironmentPrimitive::Ruin){int partIndex=0;for(const auto& part:ruin_geometry::parts(prop,zOffset)){append(propIndex,prop,PartKind::Ruin,partIndex++,part.center,part.size,0,part.surface,true);}}
        else if(prop.primitive==EnvironmentPrimitive::MarkerPillar){int partIndex=0;for(const auto& part:marker_pillar_geometry::parts(prop,zOffset)){append(propIndex,prop,PartKind::MarkerPillar,partIndex++,part.center,part.size,0,part.surface,true);}}
        else {append(propIndex,prop,PartKind::Whole,0,prop.center+Vec3{0,0,zOffset},prop.size,prop.yaw,0,room_environment::environmentPropSolid(prop),prop.primitive==EnvironmentPrimitive::Rock?GeometryKind::FacetedMesh:GeometryKind::Box);}
    }
    return result;
}

} // namespace computed_scene
