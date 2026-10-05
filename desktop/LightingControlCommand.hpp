#pragma once

#include "../game/RenderContracts.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

enum class LightingCommandKind : unsigned char { Invalid,Show,Export,Reset,Reference,SetColor,SetFogDensity,ClearChannel,SetInput };
enum class LightingInputKind : unsigned char { None,Time,Room,Phone };

struct LightingCommand {
    LightingCommandKind kind=LightingCommandKind::Invalid;
    render_contract::AtmosphereProfile reference=render_contract::AtmosphereProfile::ReadableStatic;
    render_contract::AtmosphereChannel channel=render_contract::AtmosphereChannel::Background;
    LightingInputKind input=LightingInputKind::None;
    VisualColor color{};
    float value=0.0f;
    bool live=false;
};

inline bool applyLightingCommand(render_contract::RuntimeLightingControl& control,const LightingCommand& command){
    using render_contract::AtmosphereProfile;
    switch(command.kind){
        case LightingCommandKind::Reset:control={};return true;
        case LightingCommandKind::Reference:control.reference=command.reference;return true;
        case LightingCommandKind::SetColor:render_contract::setAtmosphereColorOverride(control,command.channel,command.color);return true;
        case LightingCommandKind::SetFogDensity:render_contract::setAtmosphereFogDensityOverride(control,command.value);return true;
        case LightingCommandKind::ClearChannel:control.overrideMask&=~render_contract::atmosphereChannelBit(command.channel);return true;
        case LightingCommandKind::SetInput:
            if(command.input==LightingInputKind::Time){control.timeFixed=!command.live;if(!command.live)control.fixedInputs.time=command.value;return true;}
            if(command.input==LightingInputKind::Room){control.roomFixed=!command.live;if(!command.live)control.fixedInputs.roomIndex=std::max(1,static_cast<int>(std::lround(command.value)));return true;}
            if(command.input==LightingInputKind::Phone){control.phoneFixed=!command.live;if(!command.live)control.fixedInputs.phonePower=clampf(command.value,0.0f,1.0f);return true;}
            return false;
        default:return false;
    }
}

inline bool parseLightingFloat(const std::string& text,float& value){
    try{std::size_t used=0;value=std::stof(text,&used);return used==text.size()&&std::isfinite(value);}catch(...){return false;}
}

inline bool lightingChannel(const std::string& text,render_contract::AtmosphereChannel& channel){
    using render_contract::AtmosphereChannel;
    if(text=="background")channel=AtmosphereChannel::Background;
    else if(text=="ambient")channel=AtmosphereChannel::Ambient;
    else if(text=="sun")channel=AtmosphereChannel::Sun;
    else if(text=="fill")channel=AtmosphereChannel::Fill;
    else if(text=="phone")channel=AtmosphereChannel::Phone;
    else if(text=="fog")channel=AtmosphereChannel::Fog;
    else if(text=="fog-density")channel=AtmosphereChannel::FogDensity;
    else return false;
    return true;
}

inline LightingCommand parseLightingCommand(const std::string& input){
    std::istringstream stream(input);std::vector<std::string> words;std::string word;while(stream>>word)words.push_back(word);
    LightingCommand result;
    if(words.size()==2&&words[0]=="lighting"&&words[1]=="show"){result.kind=LightingCommandKind::Show;return result;}
    if(words.size()==2&&words[0]=="lighting"&&words[1]=="export"){result.kind=LightingCommandKind::Export;return result;}
    if(words.size()==2&&words[0]=="lighting"&&words[1]=="reset"){result.kind=LightingCommandKind::Reset;return result;}
    if(words.size()==3&&words[0]=="lighting"&&words[1]=="reference"){
        if(words[2]=="a")result.reference=render_contract::AtmosphereProfile::ReadableStatic;
        else if(words[2]=="b")result.reference=render_contract::AtmosphereProfile::ProgressiveCandidate;
        else return result;
        result.kind=LightingCommandKind::Reference;return result;
    }
    if(words.size()==3&&words[0]=="lighting"&&words[1]=="clear"&&lightingChannel(words[2],result.channel)){result.kind=LightingCommandKind::ClearChannel;return result;}
    if(words.size()==4&&words[0]=="lighting"&&words[1]=="set"&&words[2]=="fog-density"&&parseLightingFloat(words[3],result.value)){
        result.channel=render_contract::AtmosphereChannel::FogDensity;result.kind=LightingCommandKind::SetFogDensity;return result;
    }
    if(words.size()==6&&words[0]=="lighting"&&words[1]=="set"&&lightingChannel(words[2],result.channel)&&result.channel!=render_contract::AtmosphereChannel::FogDensity&&parseLightingFloat(words[3],result.color.r)&&parseLightingFloat(words[4],result.color.g)&&parseLightingFloat(words[5],result.color.b)){
        result.kind=LightingCommandKind::SetColor;return result;
    }
    if(words.size()==4&&words[0]=="lighting"&&words[1]=="input"){
        if(words[2]=="time")result.input=LightingInputKind::Time;
        else if(words[2]=="room")result.input=LightingInputKind::Room;
        else if(words[2]=="phone")result.input=LightingInputKind::Phone;
        else return result;
        if(words[3]=="live")result.live=true;
        else if(!parseLightingFloat(words[3],result.value))return LightingCommand{};
        result.kind=LightingCommandKind::SetInput;return result;
    }
    return result;
}

inline std::string serializeLightingControl(const render_contract::RuntimeLightingControl& control){
    std::ostringstream out;out.precision(9);
    out<<static_cast<int>(control.reference==render_contract::AtmosphereProfile::ProgressiveCandidate)<<' '<<control.overrideMask<<' '
        <<control.overrides.background.r<<' '<<control.overrides.background.g<<' '<<control.overrides.background.b<<' '
        <<control.overrides.ambient.r<<' '<<control.overrides.ambient.g<<' '<<control.overrides.ambient.b<<' '
        <<control.overrides.sun.r<<' '<<control.overrides.sun.g<<' '<<control.overrides.sun.b<<' '
        <<control.overrides.fill.r<<' '<<control.overrides.fill.g<<' '<<control.overrides.fill.b<<' '
        <<control.overrides.phone.r<<' '<<control.overrides.phone.g<<' '<<control.overrides.phone.b<<' '
        <<control.overrides.fog.r<<' '<<control.overrides.fog.g<<' '<<control.overrides.fog.b<<' '<<control.overrides.fogDensity<<' '
        <<static_cast<int>(control.timeFixed)<<' '<<static_cast<int>(control.roomFixed)<<' '<<static_cast<int>(control.phoneFixed)<<' '
        <<control.fixedInputs.time<<' '<<control.fixedInputs.roomIndex<<' '<<control.fixedInputs.phonePower;
    return out.str();
}

inline bool deserializeLightingControl(const std::string& encoded,render_contract::RuntimeLightingControl& control){
    std::istringstream in(encoded);int reference=0,timeFixed=0,roomFixed=0,phoneFixed=0;
    if(!(in>>reference>>control.overrideMask
        >>control.overrides.background.r>>control.overrides.background.g>>control.overrides.background.b
        >>control.overrides.ambient.r>>control.overrides.ambient.g>>control.overrides.ambient.b
        >>control.overrides.sun.r>>control.overrides.sun.g>>control.overrides.sun.b
        >>control.overrides.fill.r>>control.overrides.fill.g>>control.overrides.fill.b
        >>control.overrides.phone.r>>control.overrides.phone.g>>control.overrides.phone.b
        >>control.overrides.fog.r>>control.overrides.fog.g>>control.overrides.fog.b>>control.overrides.fogDensity
        >>timeFixed>>roomFixed>>phoneFixed>>control.fixedInputs.time>>control.fixedInputs.roomIndex>>control.fixedInputs.phonePower))return false;
    std::string trailing;if(in>>trailing)return false;
    control.reference=reference==1?render_contract::AtmosphereProfile::ProgressiveCandidate:render_contract::AtmosphereProfile::ReadableStatic;
    control.timeFixed=timeFixed!=0;control.roomFixed=roomFixed!=0;control.phoneFixed=phoneFixed!=0;return reference==0||reference==1;
}
