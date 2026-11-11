#include "ReferenceScenario.hpp"

namespace data_engine {

bool ReferenceScenario::runCompleteLoop(DataEngine& engine, int seed, int room) {
    engine.startGeneratedScenario(seed, room);
    if (!engine.prepareReferenceEncounter()) return false;
    Command melee;
    melee.melee = true;
    engine.step(melee);
    engine.step({});
    Command vacuum;
    vacuum.vacuum = true;
    for (int frame = 0; frame < 360 && engine.observe().souls == 0; ++frame) {
        engine.step(vacuum);
    }
    if (engine.observe().souls != 1) return false;
    const int required = engine.observe().requiredGoals;
    for (int goal = 0; goal < required; ++goal) {
        if (!engine.depositReferenceGoal(goal)) return false;
    }
    if (!engine.observe().roomClear || !engine.crossReferenceExit()) return false;
    return engine.chooseReferenceUpgrade(0);
}

}  // namespace data_engine
