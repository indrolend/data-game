#include <cstdio>
#include <string>

#include "AgentPlaytestProtocol.hpp"

int main() {
    using namespace agent_playtest;
    const auto step = parseCommand("step frames=120 moveX=-1 moveZ=0.25 lookX=400 sprint=on jump=1");
    const bool accepted = step.kind == CommandKind::Step && step.step.frames == 120
        && step.step.moveX == -1.0f && step.step.moveZ == 0.25f
        && step.step.lookX == 400.0f && step.step.sprint && step.step.jump;
    const bool controls = parseCommand("observe").kind == CommandKind::Observe
        && parseCommand("reset").kind == CommandKind::Reset
        && parseCommand("quit").kind == CommandKind::Quit;
    const bool rejected = parseCommand("step frames=0").kind == CommandKind::Invalid
        && parseCommand("step frames=121").kind == CommandKind::Invalid
        && parseCommand("step moveX=nan").kind == CommandKind::Invalid
        && parseCommand("step moveX=2").kind == CommandKind::Invalid
        && parseCommand("step jump=maybe").kind == CommandKind::Invalid
        && parseCommand("step frames=1 frames=2").kind == CommandKind::Invalid
        && parseCommand("step shell=whoami").kind == CommandKind::Invalid
        && parseCommand(std::string(MaximumCommandBytes + 1, 'x')).kind == CommandKind::Invalid
        && parseCommand("observe now").kind == CommandKind::Invalid;
    if (!accepted || !controls || !rejected) {
        std::fprintf(stderr, "AGENT_PLAYTEST_PROTOCOL_FAIL\n");
        return 1;
    }
    std::printf("AGENT_PLAYTEST_PROTOCOL_OK schema=%d max_frames=%d shell=ABSENT bounds=STRICT\n",
        SchemaVersion, MaximumStepFrames);
    return 0;
}
