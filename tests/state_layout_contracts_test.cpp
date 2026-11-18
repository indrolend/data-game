#include "gameplay/StateContracts.hpp"

#include <cstdio>

int main() {
    // StateContracts.hpp owns compile-time layout and protocol assertions.
    // Reaching main means those assertions compiled successfully.
    std::printf("STATE_LAYOUT game_state_bytes=%zu game_bytes=%zu target_bytes=%zu particle_bytes=%zu\n",
        sizeof(GameState),sizeof(Game),sizeof(TargetState),sizeof(ParticleState));
    return 0;
}
