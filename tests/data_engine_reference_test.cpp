#include "DataEngine.hpp"
#include "ReferenceScenario.hpp"

#include <cstdio>

namespace {

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

int main() {
    data_engine::DataEngine first;
    data_engine::DataEngine second;
    first.startGeneratedScenario(424242, 3);
    second.startGeneratedScenario(424242, 3);

    bool ok = first.observe() == second.observe();
    for (int frame = 0; frame < 240; ++frame) {
        const data_engine::Command command = commandForFrame(frame);
        ok = ok && first.step(command) == second.step(command);
    }
    ok = ok && first.events() == second.events();
    ok = ok && data_engine::DataEngine::contractDigest().size() == 16;

    data_engine::DataEngine loopA;
    data_engine::DataEngine loopB;
    ok = ok && data_engine::ReferenceScenario::runCompleteLoop(loopA, 424242, 3);
    ok = ok && data_engine::ReferenceScenario::runCompleteLoop(loopB, 424242, 3);
    ok = ok && loopA.observe() == loopB.observe();
    ok = ok && loopA.events() == loopB.events();

    if (!ok) {
        std::fprintf(stderr, "DATA_ENGINE_REFERENCE_FAIL check=deterministic_contract\n");
        return 1;
    }
    std::printf("DATA_ENGINE_REFERENCE_TEST_OK digest=%s events=%zu loop_digest=%s loop_events=%zu contract=%s\n",
                first.observe().stateDigest.c_str(), first.events().size(),
                loopA.observe().stateDigest.c_str(), loopA.events().size(),
                data_engine::DataEngine::contractDigest().c_str());
    return 0;
}
