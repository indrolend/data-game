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
        parseDeveloperCodecCommand("help extra").command==DeveloperCodecCommand::Invalid&&parseDeveloperCodecCommand("lighting show").command==DeveloperCodecCommand::Lighting;
    const auto reference=parseLightingCommand("lighting reference b"),ambient=parseLightingCommand("lighting set ambient 0.12 0.10 0.14"),time=parseLightingCommand("lighting input time 5"),bad=parseLightingCommand("lighting set ambient nope 0 0");
    render_contract::RuntimeLightingControl source;source.reference=render_contract::AtmosphereProfile::ProgressiveCandidate;render_contract::setAtmosphereFogDensityOverride(source,0.0125f);source.timeFixed=true;source.fixedInputs.time=7.25f;
    render_contract::RuntimeLightingControl decoded,applied;const bool roundtrip=deserializeLightingControl(serializeLightingControl(source),decoded);const bool appliedOk=applyLightingCommand(applied,reference)&&applyLightingCommand(applied,ambient)&&applyLightingCommand(applied,time);
    const bool lighting=bad.kind==LightingCommandKind::Invalid&&roundtrip&&decoded.reference==source.reference&&decoded.overrides.fogDensity==0.0125f&&decoded.fixedInputs.time==7.25f&&appliedOk&&applied.overrides.ambient.r==0.12f&&applied.fixedInputs.time==5.0f;
    if(!ok||!lighting){std::fprintf(stderr,"DEVELOPER_CODEC_FAILED\n");return 1;}
    std::printf("DEVELOPER_CODEC_OK allowlist=16 shell=ABSENT invalid_args=REJECTED open_input=SUPPRESSED close_input=RESTORED\n");return 0;
}
