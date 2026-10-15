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

static_assert(!unlocked(Memory::Gummy,11,7));
static_assert(unlocked(Memory::Gummy,12,0)&&unlocked(Memory::Gummy,0,8));
static_assert(!unlocked(Memory::Wireframe,34,17));
static_assert(unlocked(Memory::Wireframe,35,0)&&unlocked(Memory::Wireframe,0,18));
static_assert(!unlocked(Memory::Festive,74,29));
static_assert(unlocked(Memory::Festive,75,0)&&unlocked(Memory::Festive,0,30));

} // namespace material_unlocks
