#include "ComputedSceneObjects.hpp"
#include "Game.hpp"

#include <cassert>
#include <cstdio>
#include <set>

int main(){
    using namespace computed_scene;
    int observedObjects=0,observedCompoundProps=0;
    for(int room=1;room<=24;++room){
        const int seed=11731+room*19;
        const auto plan=room_environment::roomPlan(seed,room);
        const auto capacity=room_environment::roomGeometryCapacityPlan(plan,seed,room,ROOM_COLLIDER_COUNT);
        const auto first=environmentObjects(plan,seed,room,0,capacity);
        const auto repeat=environmentObjects(plan,seed,room,0,capacity);
        assert(first.size()==repeat.size());
        std::set<std::string> identities;
        for(std::size_t i=0;i<first.size();++i){
            const auto& object=first[i];
            assert(object.id==repeat[i].id);
            assert(object.center.x==repeat[i].center.x&&object.center.y==repeat[i].center.y&&object.center.z==repeat[i].center.z);
            assert(object.size.x>0&&object.size.y>0&&object.size.z>0);
            assert(identities.insert(object.id).second);
            const auto color=idColor(static_cast<std::uint32_t>(i+1));
            assert(idFromColor(color)==i+1);
        }
        for(int prop=0;prop<room_environment::environmentPropCount(plan);++prop)if(capacity.propIncluded[prop]){
            const auto spec=room_environment::environmentProp(plan,seed,room,prop);
            const int expected=spec.primitive==room_environment::EnvironmentPrimitive::House?house_geometry::PartCount:
                spec.primitive==room_environment::EnvironmentPrimitive::Tree?tree_geometry::TrunkPartCount+tree_geometry::CrownPartCount:
                spec.primitive==room_environment::EnvironmentPrimitive::Ruin?ruin_geometry::PartCount:
                spec.primitive==room_environment::EnvironmentPrimitive::MarkerPillar?marker_pillar_geometry::PartCount:1;
            int actual=0;for(const auto& object:first)if(object.propIndex==prop)++actual;
            assert(actual==expected);if(expected>1)++observedCompoundProps;
        }
        observedObjects+=static_cast<int>(first.size());
    }
    assert(observedObjects>0&&observedCompoundProps>0);
    std::printf("COMPUTED_SCENE_OBJECTS_OK objects=%d compound_props=%d identity=STABLE id_color=REVERSIBLE\n",observedObjects,observedCompoundProps);
}
