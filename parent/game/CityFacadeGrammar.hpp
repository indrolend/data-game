#pragma once

#include "RoomEnvironment.hpp"

#include <algorithm>
#include <cmath>

namespace city_facade {

// Derived from study of Poly Haven's CC0 Modular Urban Apartments Facade:
// authored facades read as buildings because they repeat a bay, stack a wall
// course, and terminate that repetition with a base and crown.  Runtime units
// remain calibrated to Digital Breakdown's smaller traversal scale.
struct Grammar {
    int stories=2;
    int bays=2;
    float storyHeight=1.45f;
    float bayWidth=1.35f;
    float baseHeight=0.22f;
    float corniceHeight=0.09f;
    float crownHeight=0.18f;
    float recessDepth=0.035f;
    float windowWidthRatio=0.52f;
    float windowHeightRatio=0.47f;
    int entranceBay=0;
    unsigned char family=0;
};

inline Grammar grammar(float facadeWidth,float buildingHeight,unsigned int key){
    Grammar result;
    result.family=static_cast<unsigned char>(room_environment::mix(key+17u)%3u);
    const float targetStory=result.family==0?1.42f:(result.family==1?1.58f:1.34f);
    result.stories=std::max(2,std::min(7,static_cast<int>(std::round(buildingHeight/targetStory))));
    result.storyHeight=buildingHeight/static_cast<float>(result.stories);
    const float targetBay=result.family==2?1.10f:1.38f;
    result.bays=std::max(1,std::min(6,static_cast<int>(std::round(facadeWidth/targetBay))));
    result.bayWidth=facadeWidth/static_cast<float>(result.bays);
    result.baseHeight=std::min(result.storyHeight*0.20f,0.28f);
    result.corniceHeight=std::min(result.storyHeight*0.075f,0.12f);
    result.crownHeight=result.family==1?0.26f:0.17f;
    result.recessDepth=result.family==2?0.055f:0.035f;
    result.windowWidthRatio=result.family==0?0.50f:(result.family==1?0.60f:0.42f);
    result.windowHeightRatio=result.family==2?0.55f:0.47f;
    result.entranceBay=static_cast<int>(room_environment::mix(key+31u)%static_cast<unsigned int>(result.bays));
    return result;
}

inline float bayCenter(const Grammar& grammar,int bay,float center){
    return center+(static_cast<float>(bay)+0.5f-static_cast<float>(grammar.bays)*0.5f)*grammar.bayWidth;
}

inline float storyCenter(const Grammar& grammar,int story,float bottom){
    return bottom+(static_cast<float>(story)+0.5f)*grammar.storyHeight;
}

// Occupancy is authored from the stable room/facade key rather than frame
// time.  Sparse lit rooms give the street human scale without turning every
// facade into a uniformly glowing grid or adding another simulation authority.
inline bool windowOccupied(unsigned int facadeKey,int story,int bay){
    const unsigned int offset=room_environment::mix(facadeKey+0x85ebca6bu)%5u;
    return (static_cast<unsigned int>(story*3+bay)+offset)%5u==0u;
}

inline float occupiedWindowWarmth(unsigned int facadeKey,int story,int bay){
    const unsigned int address=facadeKey+static_cast<unsigned int>((story+3)*131u+(bay+7)*977u);
    return room_environment::unit(room_environment::mix(address));
}

} // namespace city_facade
