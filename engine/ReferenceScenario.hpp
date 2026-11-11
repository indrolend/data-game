#pragma once

#include "DataEngine.hpp"

namespace data_engine {

// Compatibility harness for deterministic reference scenarios. It is kept
// separate from the product command API because fixture setup still adapts
// legacy mutable Game state internally.
class ReferenceScenario {
public:
    static bool runCompleteLoop(DataEngine& engine, int seed, int room);
};

}  // namespace data_engine
