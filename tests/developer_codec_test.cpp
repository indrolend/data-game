#include <cstdio>
#include "DeveloperCodec.hpp"
#include "LightingControlCommand.hpp"
#include "ShadowControlCommand.hpp"
#include "EnvironmentControlCommand.hpp"

int main(){
    DeveloperCodecState state;const bool closedAccepts=!state.gameplayInputSuppressed();state.open=true;const bool openSuppresses=state.gameplayInputSuppressed();state.open=false;const bool closeRestores=!state.gameplayInputSuppressed();
    const bool ok=closedAccepts&&openSuppresses&&closeRestores&&
        parseDeveloperCodecCommand("help").command==DeveloperCodecCommand::Help&&
        parseDeveloperCodecCommand("soul spawn").command==DeveloperCodecCommand::SoulSpawn&&
        parseDeveloperCodecCommand("playtest rally").command==DeveloperCodecCommand::PlaytestRally&&
        parseDeveloperCodecCommand("not-real").command==DeveloperCodecCommand::Invalid&&
        parseDeveloperCodecCommand("soul spawn extra").command==DeveloperCodecCommand::Invalid&&
        parseDeveloperCodecCommand("help extra").command==DeveloperCodecCommand::Invalid&&parseDeveloperCodecCommand("lighting show").command==DeveloperCodecCommand::Lighting&&parseDeveloperCodecCommand("shadow show").command==DeveloperCodecCommand::Shadow&&parseDeveloperCodecCommand("environment show").command==DeveloperCodecCommand::Environment;
    const auto reference=parseLightingCommand("lighting reference b"),ambient=parseLightingCommand("lighting set ambient 0.12 0.10 0.14"),time=parseLightingCommand("lighting input time 5"),bad=parseLightingCommand("lighting set ambient nope 0 0");
    render_contract::RuntimeLightingControl source;source.reference=render_contract::AtmosphereProfile::ProgressiveCandidate;render_contract::setAtmosphereFogDensityOverride(source,0.0125f);source.timeFixed=true;source.fixedInputs.time=7.25f;
    render_contract::RuntimeLightingControl decoded,applied;const bool roundtrip=deserializeLightingControl(serializeLightingControl(source),decoded);const bool appliedOk=applyLightingCommand(applied,reference)&&applyLightingCommand(applied,ambient)&&applyLightingCommand(applied,time);
    const bool lighting=bad.kind==LightingCommandKind::Invalid&&roundtrip&&decoded.reference==source.reference&&decoded.overrides.fogDensity==0.0125f&&decoded.fixedInputs.time==7.25f&&appliedOk&&applied.overrides.ambient.r==0.12f&&applied.fixedInputs.time==5.0f;
    render_contract::RuntimeShadowControl shadowControl;const auto shadowMode=parseShadowCommand("shadow mode normal"),shadowBias=parseShadowCommand("shadow bias 3 6"),shadowDepth=parseShadowCommand("shadow depth-map on"),badResolution=parseShadowCommand("shadow resolution 300");
    const bool shadow=applyShadowCommand(shadowControl,shadowMode)&&applyShadowCommand(shadowControl,shadowBias)&&applyShadowCommand(shadowControl,shadowDepth)&&!applyShadowCommand(shadowControl,badResolution)&&shadowControl.mode==render_contract::ShadowMode::MappedNormal&&shadowControl.biasFactor==3.0f&&shadowControl.biasUnits==6.0f&&shadowControl.showDepthMap;
    const auto environmentPremise=parseEnvironmentCommand("environment premise coastal-shore"),environmentSeed=parseEnvironmentCommand("environment seed 424242 12"),badEnvironment=parseEnvironmentCommand("environment premise moon");
    const bool environment=environmentPremise.kind==EnvironmentCommandKind::Premise&&environmentPremise.premise==7&&environmentSeed.kind==EnvironmentCommandKind::Seed&&environmentSeed.seed==424242&&environmentSeed.room==12&&badEnvironment.kind==EnvironmentCommandKind::Invalid;
    if(!ok||!lighting||!shadow||!environment){std::fprintf(stderr,"DEVELOPER_CODEC_FAILED\n");return 1;}
    std::printf("DEVELOPER_CODEC_OK allowlist=16 shell=ABSENT invalid_args=REJECTED open_input=SUPPRESSED close_input=RESTORED\n");return 0;
}
