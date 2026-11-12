#include "DataEngine.hpp"
#include "MultiplayerProtocol.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#include <Psapi.h>
#include <intrin.h>
#endif

namespace {

std::atomic<std::uint64_t> allocationCount{0};

struct TraceRecord {
    int frame;
    Vec3 position;
    Vec3 velocity;
    float battery;
    int souls;
    std::uint64_t digest;
};

data_engine::Command commandForFrame(int frame) {
    data_engine::Command command;
    command.moveX = frame % 240 < 80 ? 0.25f : 0.0f;
    command.moveZ = frame % 240 < 120 ? 0.70f : 0.0f;
    command.lookX = frame % 240 < 30 ? 0.35f : 0.0f;
    command.vacuum = frame % 240 >= 90 && frame % 240 < 150;
    command.sprint = frame % 240 < 60;
    command.jump = frame % 240 == 12;
    command.melee = frame % 240 == 72;
    command.shoot = frame % 240 == 170;
    return command;
}

void apply(Game& game, const data_engine::Command& command) {
    game.setTouchControls(command.moveX, command.moveZ, command.lookX, command.lookY,
                          command.vacuum, command.sprint, command.jump, command.melee,
                          command.shoot, command.cameraToggle);
    game.update(data_engine::EngineContract::fixedStepSeconds);
}

std::uint64_t cyclesNow() {
#if defined(_M_X64)
    unsigned int auxiliary = 0;
    _mm_lfence();
    const std::uint64_t value = __rdtscp(&auxiliary);
    _mm_lfence();
    return value;
#else
    return 0;
#endif
}

std::size_t workingSetBytes() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof(counters);
    return GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))
               ? counters.WorkingSetSize : 0;
#else
    return 0;
#endif
}

enum class Policy { FinalOnly, PeriodicDigest, DigestEveryTick, SelectedEveryTick, FullTrace };

const char* policyName(Policy policy) {
    switch (policy) {
        case Policy::FinalOnly: return "final_only";
        case Policy::PeriodicDigest: return "digest_every_60";
        case Policy::DigestEveryTick: return "digest_every_tick";
        case Policy::SelectedEveryTick: return "selected_every_tick";
        case Policy::FullTrace: return "full_trace";
    }
    return "unknown";
}

struct Result {
    double nanosecondsPerTick = 0.0;
    double cyclesPerTick = 0.0;
    std::uint64_t allocations = 0;
    std::size_t workingSetDelta = 0;
    std::uint64_t finalDigest = 0;
    std::uint64_t checksum = 0;
};

Result measurePolicy(Policy policy, int ticks) {
    Game game;
    game.debugStartGeneratedRoomFixture(424242, 3);
    std::vector<std::uint64_t> digests;
    std::vector<TraceRecord> trace;
    if (policy == Policy::PeriodicDigest) digests.reserve(static_cast<std::size_t>(ticks / 60 + 1));
    if (policy == Policy::DigestEveryTick) digests.reserve(static_cast<std::size_t>(ticks));
    if (policy == Policy::FullTrace) trace.reserve(static_cast<std::size_t>(ticks));
    for (int frame = 0; frame < 240; ++frame) apply(game, commandForFrame(frame));

    allocationCount.store(0, std::memory_order_relaxed);
    const std::size_t workingSetBefore = workingSetBytes();
    const std::uint64_t cycleStart = cyclesNow();
    const auto timeStart = std::chrono::steady_clock::now();
    std::uint64_t checksum = 0;
    for (int frame = 0; frame < ticks; ++frame) {
        apply(game, commandForFrame(frame));
        const GameState& state = game.state();
        if (policy == Policy::PeriodicDigest && (frame + 1) % 60 == 0) {
            digests.push_back(dbnet::authoritativeStateDigest(state));
        } else if (policy == Policy::DigestEveryTick) {
            digests.push_back(dbnet::authoritativeStateDigest(state));
        } else if (policy == Policy::SelectedEveryTick) {
            checksum += static_cast<std::uint64_t>(state.frame) +
                        static_cast<std::uint64_t>(state.player.souls) +
                        static_cast<std::uint64_t>(state.player.battery * 1000.0f);
        } else if (policy == Policy::FullTrace) {
            trace.push_back({state.frame, state.player.pos, state.player.vel, state.player.battery,
                             state.player.souls, dbnet::authoritativeStateDigest(state)});
        }
    }
    const auto timeEnd = std::chrono::steady_clock::now();
    const std::uint64_t cycleEnd = cyclesNow();
    const std::size_t workingSetAfter = workingSetBytes();
    for (std::uint64_t digest : digests) checksum ^= digest;
    for (const TraceRecord& record : trace) checksum ^= record.digest + static_cast<std::uint64_t>(record.frame);
    return {
        std::chrono::duration<double, std::nano>(timeEnd - timeStart).count() / ticks,
        static_cast<double>(cycleEnd - cycleStart) / ticks,
        allocationCount.load(std::memory_order_relaxed),
        workingSetAfter >= workingSetBefore ? workingSetAfter - workingSetBefore : 0,
        dbnet::authoritativeStateDigest(game.state()),
        checksum,
    };
}

Result measureDataEngine(int ticks) {
    data_engine::DataEngine engine;
    engine.startGeneratedScenario(424242, 3);
    for (int frame = 0; frame < 240; ++frame) engine.step(commandForFrame(frame));
    allocationCount.store(0, std::memory_order_relaxed);
    const std::size_t workingSetBefore = workingSetBytes();
    const std::uint64_t cycleStart = cyclesNow();
    const auto timeStart = std::chrono::steady_clock::now();
    std::uint64_t checksum = 0;
    for (int frame = 0; frame < ticks; ++frame) {
        const auto observation = engine.step(commandForFrame(frame));
        checksum += static_cast<std::uint64_t>(observation.frame) + observation.stateDigest.size();
    }
    const auto timeEnd = std::chrono::steady_clock::now();
    const std::uint64_t cycleEnd = cyclesNow();
    const std::size_t workingSetAfter = workingSetBytes();
    return {
        std::chrono::duration<double, std::nano>(timeEnd - timeStart).count() / ticks,
        static_cast<double>(cycleEnd - cycleStart) / ticks,
        allocationCount.load(std::memory_order_relaxed),
        workingSetAfter >= workingSetBefore ? workingSetAfter - workingSetBefore : 0,
        dbnet::authoritativeStateDigest(engine.authoritativeState()),
        checksum,
    };
}

}  // namespace

void* operator new(std::size_t size) {
    allocationCount.fetch_add(1, std::memory_order_relaxed);
    if (void* memory = std::malloc(size)) return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }

int main(int argc, char** argv) {
    const int ticks = argc > 1 ? (std::max)(1, std::atoi(argv[1])) : 12000;
#if defined(_WIN32)
    SetThreadAffinityMask(GetCurrentThread(), 1);
#endif
    std::printf("TRANSITION_BENCHMARK identity=%s ticks=%d sizeof_game_state=%zu sizeof_command=%zu sizeof_observation=%zu\n",
                data_engine::EngineContract::identity(), ticks, sizeof(GameState),
                sizeof(data_engine::Command), sizeof(data_engine::Observation));
    std::uint64_t referenceDigest = 0;
    for (Policy policy : {Policy::FinalOnly, Policy::PeriodicDigest, Policy::DigestEveryTick,
                          Policy::SelectedEveryTick, Policy::FullTrace}) {
        const Result result = measurePolicy(policy, ticks);
        if (referenceDigest == 0) referenceDigest = result.finalDigest;
        std::printf("TRANSITION_POLICY name=%s ns_per_tick=%.3f cycles_per_tick=%.3f allocations=%llu working_set_delta=%zu final_digest=%016llx equivalent=%d checksum=%016llx\n",
                    policyName(policy), result.nanosecondsPerTick, result.cyclesPerTick,
                    static_cast<unsigned long long>(result.allocations), result.workingSetDelta,
                    static_cast<unsigned long long>(result.finalDigest), result.finalDigest == referenceDigest,
                    static_cast<unsigned long long>(result.checksum));
    }
    const Result engine = measureDataEngine(ticks);
    std::printf("TRANSITION_POLICY name=data_engine_step ns_per_tick=%.3f cycles_per_tick=%.3f allocations=%llu working_set_delta=%zu final_digest=%016llx equivalent=%d checksum=%016llx\n",
                engine.nanosecondsPerTick, engine.cyclesPerTick,
                static_cast<unsigned long long>(engine.allocations), engine.workingSetDelta,
                static_cast<unsigned long long>(engine.finalDigest), engine.finalDigest == referenceDigest,
                static_cast<unsigned long long>(engine.checksum));
    std::printf("TRANSITION_COUNTERS retired_instructions=unavailable branches=unavailable cache_misses=unavailable reason=user_mode_counter_interface_not_available\n");
    return engine.finalDigest == referenceDigest ? 0 : 1;
}
