#pragma once

#include "Game.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace data_engine {

struct EngineContract {
    static constexpr int schema = 1;
    static constexpr float fixedStepSeconds = 1.0f / 60.0f;

    static const char* identity() {
        return "data-engine-contract-v1;fixed_hz=60;simulation=game";
    }
};

struct Command {
    float moveX = 0.0f;
    float moveZ = 0.0f;
    float lookX = 0.0f;
    float lookY = 0.0f;
    bool vacuum = false;
    bool sprint = false;
    bool jump = false;
    bool melee = false;
    bool shoot = false;
    bool cameraToggle = false;
};

enum class EventType {
    RunStarted,
    SoulCaptured,
    GoalFilled,
    RoomCleared,
    RoomChanged,
    UpgradeOffered,
    PlayerDied,
    PlayerRestarted,
};

struct Event {
    EventType type = EventType::RunStarted;
    int frame = 0;
    int value = 0;

    bool operator==(const Event& other) const {
        return type == other.type && frame == other.frame && value == other.value;
    }
};

struct Observation {
    int frame = 0;
    int room = 0;
    int souls = 0;
    int filledGoals = 0;
    int requiredGoals = 0;
    bool started = false;
    bool dead = false;
    bool roomClear = false;
    bool upgradeOffered = false;
    std::string stateDigest;

    bool operator==(const Observation& other) const {
        return frame == other.frame && room == other.room && souls == other.souls &&
               filledGoals == other.filledGoals && requiredGoals == other.requiredGoals &&
               started == other.started && dead == other.dead && roomClear == other.roomClear &&
               upgradeOffered == other.upgradeOffered &&
               stateDigest == other.stateDigest;
    }
};

class DataEngine {
public:
    void reset();
    void startGeneratedScenario(int seed, int room);
    Observation step(const Command& command);

    Observation observe() const;
    const std::vector<Event>& events() const { return events_; }
    const GameState& authoritativeState() const { return game_.state(); }

    static std::string contractDigest();
    static const char* eventName(EventType type);

private:
    friend class ReferenceScenario;
    Game game_;
    std::vector<Event> events_;
    bool hasPrevious_ = false;
    int previousRoom_ = 0;
    int previousSouls_ = 0;
    int previousFilledGoals_ = 0;
    bool previousStarted_ = false;
    bool previousDead_ = false;
    bool previousRoomClear_ = false;
    bool previousUpgradeOffered_ = false;

    void captureEvents();
    void rememberState();
    bool prepareReferenceEncounter();
    bool depositReferenceGoal(int goalIndex);
    bool crossReferenceExit();
    bool chooseReferenceUpgrade(int track);
};

}  // namespace data_engine
