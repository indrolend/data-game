#include "DataEngine.hpp"
#include "ReferenceScenario.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

int integerArgument(int argc, char** argv, const char* name, int fallback) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], name) == 0) return std::atoi(argv[i + 1]);
    }
    return fallback;
}

const char* stringArgument(int argc, char** argv, const char* name, const char* fallback) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], name) == 0) return argv[i + 1];
    }
    return fallback;
}

data_engine::Command commandForFrame(int frame) {
    data_engine::Command command;
    command.moveX = frame < 80 ? 0.25f : 0.0f;
    command.moveZ = frame < 120 ? 0.70f : 0.0f;
    command.lookX = frame < 30 ? 0.35f : 0.0f;
    command.vacuum = frame >= 90 && frame < 150;
    command.sprint = frame < 60;
    command.jump = frame == 12;
    command.melee = frame == 72;
    command.shoot = frame == 170;
    return command;
}

}  // namespace

int main(int argc, char** argv) {
    const int seed = integerArgument(argc, argv, "--seed", 424242);
    const int room = integerArgument(argc, argv, "--room", 3);
    const int frames = integerArgument(argc, argv, "--frames", 240);
    const char* scenario = stringArgument(argc, argv, "--scenario", "deterministic-input-v1");
    if (frames < 0) {
        std::fprintf(stderr, "DATA_ENGINE_REFERENCE_FAIL reason=invalid_frames\n");
        return 2;
    }

    data_engine::DataEngine engine;
    if (std::strcmp(scenario, "complete-loop-v1") == 0) {
        data_engine::DataEngine replay;
        const bool completed = data_engine::ReferenceScenario::runCompleteLoop(engine, seed, room);
        const bool replayed = data_engine::ReferenceScenario::runCompleteLoop(replay, seed, room);
        const bool identical = completed && replayed && engine.observe() == replay.observe() &&
                               engine.events() == replay.events();
        std::printf(
            "{\"type\":\"manifest\",\"engine_schema\":1,\"contract_schema\":%d,"
            "\"contract_digest\":\"%s\",\"scenario\":\"complete-loop-v1\","
            "\"seed\":%d,\"room\":%d}\n",
            data_engine::EngineContract::schema, data_engine::DataEngine::contractDigest().c_str(), seed, room);
        for (const data_engine::Event& event : engine.events()) {
            std::printf("{\"type\":\"event\",\"event\":\"%s\",\"frame\":%d,\"value\":%d}\n",
                        data_engine::DataEngine::eventName(event.type), event.frame, event.value);
        }
        const data_engine::Observation final = engine.observe();
        std::printf("DATA_ENGINE_COMPLETE_LOOP_%s frame=%d room=%d digest=%s events=%zu replay_identical=%d\n",
                    identical ? "OK" : "FAIL", final.frame, final.room, final.stateDigest.c_str(),
                    engine.events().size(), identical ? 1 : 0);
        return identical ? 0 : 1;
    }
    if (std::strcmp(scenario, "deterministic-input-v1") != 0) {
        std::fprintf(stderr, "DATA_ENGINE_REFERENCE_FAIL reason=unknown_scenario scenario=%s\n", scenario);
        return 2;
    }
    engine.startGeneratedScenario(seed, room);
    std::printf(
        "{\"type\":\"manifest\",\"engine_schema\":1,\"contract_schema\":%d,"
        "\"contract_digest\":\"%s\",\"scenario\":\"deterministic-input-v1\","
        "\"seed\":%d,\"room\":%d,\"frames\":%d}\n",
        data_engine::EngineContract::schema, data_engine::DataEngine::contractDigest().c_str(),
        seed, room, frames);

    for (int frame = 0; frame < frames; ++frame) {
        const data_engine::Observation observation = engine.step(commandForFrame(frame));
        if ((frame + 1) % 60 == 0 || frame + 1 == frames) {
            std::printf(
                "{\"type\":\"observation\",\"frame\":%d,\"room\":%d,"
                "\"souls\":%d,\"started\":%s,\"dead\":%s,\"state_digest\":\"%s\"}\n",
                observation.frame, observation.room, observation.souls,
                observation.started ? "true" : "false", observation.dead ? "true" : "false",
                observation.stateDigest.c_str());
        }
    }
    for (const data_engine::Event& event : engine.events()) {
        std::printf("{\"type\":\"event\",\"event\":\"%s\",\"frame\":%d,\"value\":%d}\n",
                    data_engine::DataEngine::eventName(event.type), event.frame, event.value);
    }
    const data_engine::Observation final = engine.observe();
    std::printf("DATA_ENGINE_REFERENCE_OK frame=%d digest=%s events=%zu\n",
                final.frame, final.stateDigest.c_str(), engine.events().size());
    return 0;
}
