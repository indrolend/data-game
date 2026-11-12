#pragma once
#include "../game/RenderContracts.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

enum class ShadowCommandKind : unsigned char { Invalid, Show, Reset, Mode, Strength, Resolution, Bias, Coverage, DepthMap, GeometryOnly };
struct ShadowCommand {
    ShadowCommandKind kind=ShadowCommandKind::Invalid;
    render_contract::ShadowMode mode=render_contract::ShadowMode::Automatic;
    float first=0.0f,second=0.0f;
    int integer=0;
    bool enabled=false;
};

inline bool parseShadowFloat(const std::string& text,float& value){try{std::size_t used=0;value=std::stof(text,&used);return used==text.size()&&std::isfinite(value);}catch(...){return false;}}
inline bool parseShadowInteger(const std::string& text,int& value){try{std::size_t used=0;value=std::stoi(text,&used);return used==text.size();}catch(...){return false;}}
inline bool parseShadowToggle(const std::string& text,bool& value){if(text=="on"){value=true;return true;}if(text=="off"){value=false;return true;}return false;}
inline ShadowCommand parseShadowCommand(const std::string& input){
    std::istringstream stream(input);std::vector<std::string> words;std::string word;while(stream>>word)words.push_back(word);ShadowCommand result;
    if(words.size()==2&&words[0]=="shadow"&&words[1]=="show"){result.kind=ShadowCommandKind::Show;return result;}
    if(words.size()==2&&words[0]=="shadow"&&words[1]=="reset"){result.kind=ShadowCommandKind::Reset;return result;}
    if(words.size()==3&&words[0]=="shadow"&&words[1]=="mode"){
        using render_contract::ShadowMode;
        if(words[2]=="auto")result.mode=ShadowMode::Automatic;else if(words[2]=="off")result.mode=ShadowMode::Off;else if(words[2]=="cheap")result.mode=ShadowMode::Cheap;else if(words[2]=="normal")result.mode=ShadowMode::MappedNormal;else if(words[2]=="pretty")result.mode=ShadowMode::MappedPretty;else return result;
        result.kind=ShadowCommandKind::Mode;return result;
    }
    if(words.size()==3&&words[0]=="shadow"&&words[1]=="strength"&&parseShadowFloat(words[2],result.first)){result.kind=ShadowCommandKind::Strength;return result;}
    if(words.size()==3&&words[0]=="shadow"&&words[1]=="resolution"&&parseShadowInteger(words[2],result.integer)){result.kind=ShadowCommandKind::Resolution;return result;}
    if(words.size()==4&&words[0]=="shadow"&&words[1]=="bias"&&parseShadowFloat(words[2],result.first)&&parseShadowFloat(words[3],result.second)){result.kind=ShadowCommandKind::Bias;return result;}
    if(words.size()==3&&words[0]=="shadow"&&words[1]=="coverage"&&parseShadowFloat(words[2],result.first)){result.kind=ShadowCommandKind::Coverage;return result;}
    if(words.size()==3&&words[0]=="shadow"&&words[1]=="depth-map"&&parseShadowToggle(words[2],result.enabled)){result.kind=ShadowCommandKind::DepthMap;return result;}
    if(words.size()==3&&words[0]=="shadow"&&words[1]=="geometry"&&parseShadowToggle(words[2],result.enabled)){result.kind=ShadowCommandKind::GeometryOnly;return result;}
    return result;
}
inline bool applyShadowCommand(render_contract::RuntimeShadowControl& control,const ShadowCommand& command){
    switch(command.kind){
        case ShadowCommandKind::Reset:control={};return true;
        case ShadowCommandKind::Mode:control.mode=command.mode;return true;
        case ShadowCommandKind::Strength:control.strengthScale=clampf(command.first,0.0f,3.0f);return true;
        case ShadowCommandKind::Resolution:if(command.integer!=0&&command.integer!=256&&command.integer!=512&&command.integer!=1024&&command.integer!=2048&&command.integer!=4096)return false;control.resolution=command.integer;return true;
        case ShadowCommandKind::Bias:control.biasFactor=clampf(command.first,0.0f,16.0f);control.biasUnits=clampf(command.second,0.0f,32.0f);return true;
        case ShadowCommandKind::Coverage:control.coverageScale=clampf(command.first,0.25f,4.0f);return true;
        case ShadowCommandKind::DepthMap:control.showDepthMap=command.enabled;return true;
        case ShadowCommandKind::GeometryOnly:control.geometryOnly=command.enabled;return true;
        default:return false;
    }
}
inline const char* shadowModeName(render_contract::ShadowMode mode){using render_contract::ShadowMode;switch(mode){case ShadowMode::Automatic:return "auto";case ShadowMode::Off:return "off";case ShadowMode::Cheap:return "cheap";case ShadowMode::MappedNormal:return "normal";case ShadowMode::MappedPretty:return "pretty";}return "unknown";}
inline const char* shadowQualityName(render_contract::ShadowQuality quality){using render_contract::ShadowQuality;switch(quality){case ShadowQuality::Off:return "off";case ShadowQuality::Cheap:return "cheap";case ShadowQuality::MappedNormal:return "mapped-normal";case ShadowQuality::MappedPretty:return "mapped-pretty";}return "unknown";}
