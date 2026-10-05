#include <cstdio>
#include "DeveloperCodec.hpp"
#include "LightingControlCommand.hpp"

int main(){
    DeveloperCodecState state;const bool closedAccepts=!state.gameplayInputSuppressed();state.open=true;const bool openSuppresses=state.gameplayInputSuppressed();state.open=false;const bool closeRestores=!state.gameplayInputSuppressed();
    const bool ok=closedAccepts&&openSuppresses&&closeRestores&&
        parseDeveloperCodecCommand("help").command==DeveloperCodecCommand::Help&&
        parseDeveloperCodecCommand("soul spawn").command==DeveloperCodecCommand::SoulSpawn&&
        parseDeveloperCodecCommand("playtest rally").command==DeveloperCodecCommand::PlaytestRally&&
        parseDeveloperCodecCommand("not-real").command==DeveloperCodecCommand::Invalid&&
        parseDeveloperCodecCommand("soul spawn extra").command==DeveloperCodecCommand::Invalid&&
        parseDeveloperCodecCommand("help extra").command==DeveloperCodecCommand::Invalid&&
        parseDeveloperCodecCommand("lighting show").command==DeveloperCodecCommand::Lighting;
    const auto reference=parseLightingCommand("lighting reference b");
    const auto ambient=parseLightingCommand("lighting set ambient 0.12 0.10 0.14");
    const auto time=parseLightingCommand("lighting input time 5");
    const auto bad=parseLightingCommand("lighting set ambient nope 0 0");
    render_contract::RuntimeLightingControl serializedControl;serializedControl.reference=render_contract::AtmosphereProfile::ProgressiveCandidate;render_contract::setAtmosphereFogDensityOverride(serializedControl,0.0125f);serializedControl.timeFixed=true;serializedControl.fixedInputs.time=7.25f;
    render_contract::RuntimeLightingControl decodedControl;const bool serialized=deserializeLightingControl(serializeLightingControl(serializedControl),decodedControl);
    render_contract::RuntimeLightingControl appliedControl;const bool applied=applyLightingCommand(appliedControl,reference)&&applyLightingCommand(appliedControl,ambient)&&applyLightingCommand(appliedControl,time);
    const bool lightingCommands=reference.kind==LightingCommandKind::Reference&&reference.reference==render_contract::AtmosphereProfile::ProgressiveCandidate&&
        ambient.kind==LightingCommandKind::SetColor&&ambient.channel==render_contract::AtmosphereChannel::Ambient&&ambient.color.r==0.12f&&
        time.kind==LightingCommandKind::SetInput&&time.input==LightingInputKind::Time&&time.value==5.0f&&bad.kind==LightingCommandKind::Invalid&&serialized&&decodedControl.reference==render_contract::AtmosphereProfile::ProgressiveCandidate&&decodedControl.overrides.fogDensity==0.0125f&&decodedControl.fixedInputs.time==7.25f&&applied&&appliedControl.reference==render_contract::AtmosphereProfile::ProgressiveCandidate&&appliedControl.overrides.ambient.r==0.12f&&appliedControl.fixedInputs.time==5.0f;
    const bool allOk=ok&&lightingCommands;
    if(!allOk){std::fprintf(stderr,"DEVELOPER_CODEC_FAILED\n");return 1;}
    std::printf("DEVELOPER_CODEC_OK allowlist=17 lighting=CONTROLLED shell=ABSENT invalid_args=REJECTED open_input=SUPPRESSED close_input=RESTORED\n");return 0;
}
