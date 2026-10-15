#pragma once

struct DesktopPersistencePolicyInputs {
    bool evidenceScenario=false;
    bool agentPlaytest=false;
    bool captureFrame=false;
    bool captureDemo=false;
    bool soulLifecycle=false;
    bool enemyMotion=false;
    bool roomInspectorSmoke=false;
    bool multiplayerTest=false;
    bool combatRenderStress=false;
    bool combatCrowdStress=false;
};

constexpr bool isolatesPersistentSave(const DesktopPersistencePolicyInputs& inputs) noexcept {
    // Machine-driven fixtures must start from source-defined state. Loading a
    // player's progression/settings makes their output machine-dependent and
    // needlessly exposes the real save to diagnostic code paths.
    return inputs.evidenceScenario||inputs.agentPlaytest||inputs.captureFrame||inputs.captureDemo||
        inputs.soulLifecycle||inputs.enemyMotion||inputs.roomInspectorSmoke||inputs.multiplayerTest||
        inputs.combatRenderStress||inputs.combatCrowdStress;
}
