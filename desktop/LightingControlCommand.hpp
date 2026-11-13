#pragma once
#include "../game/RenderContracts.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

enum class LightingCommandKind : unsigned char { Invalid,Show,Export,Reset,Reference,SetColor,SetFogDensity,ClearChannel,SetInput,SetLocalLight,SetLocalIntensity,IsolateLocalLight,EnableAllLocalLights };
enum class LightingInputKind : unsigned char { None,Time,Room,Phone };
struct LightingCommand { LightingCommandKind kind=LightingCommandKind::Invalid;render_contract::AtmosphereProfile reference=render_contract::AtmosphereProfile::ReadableStatic;render_contract::AtmosphereChannel channel=render_contract::AtmosphereChannel::Background;LightingInputKind input=LightingInputKind::None;VisualColor color{};float value=0.0f;int lightIndex=-1;bool live=false,enabled=false; };

inline bool applyLightingCommand(render_contract::RuntimeLightingControl& control,const LightingCommand& command){
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
        case LightingCommandKind::SetLocalLight:if(command.lightIndex<0||command.lightIndex>=3)return false;if(command.enabled)control.localLightMask|=static_cast<std::uint8_t>(1u<<command.lightIndex);else control.localLightMask&=static_cast<std::uint8_t>(~(1u<<command.lightIndex));return true;
        case LightingCommandKind::SetLocalIntensity:if(command.lightIndex<0||command.lightIndex>=3)return false;control.localLightIntensity[static_cast<std::size_t>(command.lightIndex)]=clampf(command.value,0.0f,4.0f);return true;
        case LightingCommandKind::IsolateLocalLight:if(command.lightIndex<0||command.lightIndex>=3)return false;control.localLightMask=static_cast<std::uint8_t>(1u<<command.lightIndex);return true;
        case LightingCommandKind::EnableAllLocalLights:control.localLightMask=0x7u;return true;
        default:return false;
    }
}
inline bool parseLightingFloat(const std::string& text,float& value){try{std::size_t used=0;value=std::stof(text,&used);return used==text.size()&&std::isfinite(value);}catch(...){return false;}}
inline bool lightingChannel(const std::string& text,render_contract::AtmosphereChannel& channel){using render_contract::AtmosphereChannel;if(text=="background")channel=AtmosphereChannel::Background;else if(text=="ambient")channel=AtmosphereChannel::Ambient;else if(text=="sun")channel=AtmosphereChannel::Sun;else if(text=="fill")channel=AtmosphereChannel::Fill;else if(text=="phone")channel=AtmosphereChannel::Phone;else if(text=="fog")channel=AtmosphereChannel::Fog;else if(text=="fog-density")channel=AtmosphereChannel::FogDensity;else return false;return true;}
inline bool lightingIndex(const std::string& text,int& index){float value=0.0f;if(!parseLightingFloat(text,value)||std::floor(value)!=value||value<0.0f||value>2.0f)return false;index=static_cast<int>(value);return true;}
inline LightingCommand parseLightingCommand(const std::string& input){
    std::istringstream stream(input);std::vector<std::string> words;std::string word;while(stream>>word)words.push_back(word);LightingCommand result;
    if(words.size()==2&&words[0]=="lighting"&&words[1]=="show"){result.kind=LightingCommandKind::Show;return result;}
    if(words.size()==2&&words[0]=="lighting"&&words[1]=="export"){result.kind=LightingCommandKind::Export;return result;}
    if(words.size()==2&&words[0]=="lighting"&&words[1]=="reset"){result.kind=LightingCommandKind::Reset;return result;}
    if(words.size()==3&&words[0]=="lighting"&&words[1]=="reference"){if(words[2]=="a")result.reference=render_contract::AtmosphereProfile::ReadableStatic;else if(words[2]=="b")result.reference=render_contract::AtmosphereProfile::ProgressiveCandidate;else return result;result.kind=LightingCommandKind::Reference;return result;}
    if(words.size()==3&&words[0]=="lighting"&&words[1]=="clear"&&lightingChannel(words[2],result.channel)){result.kind=LightingCommandKind::ClearChannel;return result;}
    if(words.size()==4&&words[0]=="lighting"&&words[1]=="set"&&words[2]=="fog-density"&&parseLightingFloat(words[3],result.value)){result.channel=render_contract::AtmosphereChannel::FogDensity;result.kind=LightingCommandKind::SetFogDensity;return result;}
    if(words.size()==6&&words[0]=="lighting"&&words[1]=="set"&&lightingChannel(words[2],result.channel)&&result.channel!=render_contract::AtmosphereChannel::FogDensity&&parseLightingFloat(words[3],result.color.r)&&parseLightingFloat(words[4],result.color.g)&&parseLightingFloat(words[5],result.color.b)){result.kind=LightingCommandKind::SetColor;return result;}
    if(words.size()==4&&words[0]=="lighting"&&words[1]=="input"){if(words[2]=="time")result.input=LightingInputKind::Time;else if(words[2]=="room")result.input=LightingInputKind::Room;else if(words[2]=="phone")result.input=LightingInputKind::Phone;else return result;if(words[3]=="live")result.live=true;else if(!parseLightingFloat(words[3],result.value))return {};result.kind=LightingCommandKind::SetInput;return result;}
    if(words.size()==3&&words[0]=="lighting"&&words[1]=="lights"&&words[2]=="all"){result.kind=LightingCommandKind::EnableAllLocalLights;return result;}
    if(words.size()==4&&words[0]=="lighting"&&words[1]=="lights"&&words[2]=="isolate"&&lightingIndex(words[3],result.lightIndex)){result.kind=LightingCommandKind::IsolateLocalLight;return result;}
    if(words.size()==4&&words[0]=="lighting"&&words[1]=="light"&&lightingIndex(words[2],result.lightIndex)&&(words[3]=="on"||words[3]=="off")){result.kind=LightingCommandKind::SetLocalLight;result.enabled=words[3]=="on";return result;}
    if(words.size()==5&&words[0]=="lighting"&&words[1]=="light"&&lightingIndex(words[2],result.lightIndex)&&words[3]=="intensity"&&parseLightingFloat(words[4],result.value)){result.kind=LightingCommandKind::SetLocalIntensity;return result;}
    return result;
}
inline std::string serializeLightingControl(const render_contract::RuntimeLightingControl& c){std::ostringstream o;o.precision(9);o<<static_cast<int>(c.reference==render_contract::AtmosphereProfile::ProgressiveCandidate)<<' '<<c.overrideMask<<' '<<c.overrides.background.r<<' '<<c.overrides.background.g<<' '<<c.overrides.background.b<<' '<<c.overrides.ambient.r<<' '<<c.overrides.ambient.g<<' '<<c.overrides.ambient.b<<' '<<c.overrides.sun.r<<' '<<c.overrides.sun.g<<' '<<c.overrides.sun.b<<' '<<c.overrides.fill.r<<' '<<c.overrides.fill.g<<' '<<c.overrides.fill.b<<' '<<c.overrides.phone.r<<' '<<c.overrides.phone.g<<' '<<c.overrides.phone.b<<' '<<c.overrides.fog.r<<' '<<c.overrides.fog.g<<' '<<c.overrides.fog.b<<' '<<c.overrides.fogDensity<<' '<<static_cast<int>(c.timeFixed)<<' '<<static_cast<int>(c.roomFixed)<<' '<<static_cast<int>(c.phoneFixed)<<' '<<c.fixedInputs.time<<' '<<c.fixedInputs.roomIndex<<' '<<c.fixedInputs.phonePower<<' '<<static_cast<int>(c.localLightMask)<<' '<<c.localLightIntensity[0]<<' '<<c.localLightIntensity[1]<<' '<<c.localLightIntensity[2];return o.str();}
inline bool deserializeLightingControl(const std::string& encoded,render_contract::RuntimeLightingControl& c){std::istringstream in(encoded);int reference=0,timeFixed=0,roomFixed=0,phoneFixed=0;if(!(in>>reference>>c.overrideMask>>c.overrides.background.r>>c.overrides.background.g>>c.overrides.background.b>>c.overrides.ambient.r>>c.overrides.ambient.g>>c.overrides.ambient.b>>c.overrides.sun.r>>c.overrides.sun.g>>c.overrides.sun.b>>c.overrides.fill.r>>c.overrides.fill.g>>c.overrides.fill.b>>c.overrides.phone.r>>c.overrides.phone.g>>c.overrides.phone.b>>c.overrides.fog.r>>c.overrides.fog.g>>c.overrides.fog.b>>c.overrides.fogDensity>>timeFixed>>roomFixed>>phoneFixed>>c.fixedInputs.time>>c.fixedInputs.roomIndex>>c.fixedInputs.phonePower))return false;int mask=7;if(in>>mask){if(!(in>>c.localLightIntensity[0]>>c.localLightIntensity[1]>>c.localLightIntensity[2]))return false;c.localLightMask=static_cast<std::uint8_t>(mask&7);}in.clear();std::string trailing;if(in>>trailing)return false;c.reference=reference==1?render_contract::AtmosphereProfile::ProgressiveCandidate:render_contract::AtmosphereProfile::ReadableStatic;c.timeFixed=timeFixed!=0;c.roomFixed=roomFixed!=0;c.phoneFixed=phoneFixed!=0;return reference==0||reference==1;}
