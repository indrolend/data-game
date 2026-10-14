#pragma once

#include <cstdint>

namespace material_unlocks {

enum class Memory : unsigned char { Gummy, Wireframe, Festive };

struct Requirement { std::uint64_t lifetimeRooms; int bestRoom; const char* name; };

constexpr Requirement requirement(Memory memory) {
    switch(memory){
        case Memory::Gummy:return {12,8,"GUMMY MEMORY"};
        case Memory::Wireframe:return {35,18,"WIREFRAME MEMORY"};
        case Memory::Festive:return {75,30,"FESTIVE MEMORY"};
    }
    return {};
}

constexpr bool unlocked(Memory memory,std::uint64_t lifetimeRooms,int bestRoom) {
    const Requirement target=requirement(memory);
    return lifetimeRooms>=target.lifetimeRooms||bestRoom>=target.bestRoom;
}

constexpr bool newlyUnlocked(Memory memory,std::uint64_t beforeLifetime,int beforeBest,std::uint64_t afterLifetime,int afterBest) {
    return !unlocked(memory,beforeLifetime,beforeBest)&&unlocked(memory,afterLifetime,afterBest);
}

} // namespace material_unlocks
