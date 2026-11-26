#pragma once

#include <algorithm>
#include <array>
#include <sstream>
#include <string>
#include <vector>

enum class DeveloperCodecCommand : unsigned char {
    Invalid, Help, State, SoulSpawn, BatteryFull,
    EnemiesOn, EnemiesOff, EnemiesToggle,
    RoomNext, RoomReroll,
    CollidersShow, CollidersHide, CollidersToggle,
    PlaytestRally, PlaytestTraversal, PlaytestSlope, PlaytestRooms,
    PlaytestField, PlaytestTv, PlaytestTvEnter, CaptureFrame,
    Lighting, Shadow, Environment
};

struct DeveloperCodecQuickAction { const char* label;const char* command; };
inline constexpr std::array<DeveloperCodecQuickAction,15> DeveloperCodecQuickActions{{
    {"STATE SNAPSHOT","state"}, {"CAPTURE FRAME","capture frame"},
    {"SPAWN SOUL","soul spawn"}, {"FILL BATTERY","battery full"},
    {"TOGGLE ENEMIES","enemies toggle"}, {"TOGGLE COLLIDERS","colliders toggle"},
    {"NEXT ROOM","room next"}, {"REROLL ROOM","room reroll"},
    {"RALLY LAB","playtest rally"}, {"TRAVERSAL LAB","playtest traversal"},
    {"SLOPE LAB","playtest slope"}, {"ROOM INSPECTOR","playtest rooms"},
    {"FIELD FIXTURE","playtest field"}, {"TV ENTRANCE","playtest tv"},
    {"ENTER TV ROOM","playtest tv-enter"}
}};

inline int moveDeveloperCodecQuickSelection(int current,int horizontal,int vertical){
    const int count=static_cast<int>(DeveloperCodecQuickActions.size());
    current=std::max(0,std::min(count-1,current));
    const int column=current%2;
    if(horizontal!=0){
        const int candidate=current+(horizontal>0?1:-1);
        return candidate>=0&&candidate<count&&candidate/2==current/2?candidate:current;
    }
    if(vertical==0)return current;
    const int columnCount=(count+1-column)/2;
    const int row=current/2;
    const int nextRow=(row+(vertical>0?1:-1)+columnCount)%columnCount;
    return nextRow*2+column;
}

struct DeveloperCodecParseResult {
    DeveloperCodecCommand command=DeveloperCodecCommand::Invalid;
    bool recognizedVerb=false;
};

inline DeveloperCodecParseResult parseDeveloperCodecCommand(const std::string& input){
    std::istringstream stream(input);std::vector<std::string> words;std::string word;
    while(stream>>word)words.push_back(word);
    if(words.empty())return {};
    const auto exact=[&](const char* a,const char* b=nullptr){return words.size()==(b?2u:1u)&&words[0]==a&&(!b||words[1]==b);};
    if(exact("help"))return {DeveloperCodecCommand::Help,true};
    if(exact("state"))return {DeveloperCodecCommand::State,true};
    if(exact("soul","spawn"))return {DeveloperCodecCommand::SoulSpawn,true};
    if(exact("battery","full"))return {DeveloperCodecCommand::BatteryFull,true};
    if(exact("enemies","on"))return {DeveloperCodecCommand::EnemiesOn,true};
    if(exact("enemies","off"))return {DeveloperCodecCommand::EnemiesOff,true};
    if(exact("enemies","toggle"))return {DeveloperCodecCommand::EnemiesToggle,true};
    if(exact("room","next"))return {DeveloperCodecCommand::RoomNext,true};
    if(exact("room","reroll"))return {DeveloperCodecCommand::RoomReroll,true};
    if(exact("colliders","show"))return {DeveloperCodecCommand::CollidersShow,true};
    if(exact("colliders","hide"))return {DeveloperCodecCommand::CollidersHide,true};
    if(exact("colliders","toggle"))return {DeveloperCodecCommand::CollidersToggle,true};
    if(exact("playtest","rally"))return {DeveloperCodecCommand::PlaytestRally,true};
    if(exact("playtest","traversal"))return {DeveloperCodecCommand::PlaytestTraversal,true};
    if(exact("playtest","slope"))return {DeveloperCodecCommand::PlaytestSlope,true};
    if(exact("playtest","rooms"))return {DeveloperCodecCommand::PlaytestRooms,true};
    if(exact("playtest","field"))return {DeveloperCodecCommand::PlaytestField,true};
    if(exact("playtest","tv"))return {DeveloperCodecCommand::PlaytestTv,true};
    if(exact("playtest","tv-enter"))return {DeveloperCodecCommand::PlaytestTvEnter,true};
    if(exact("capture","frame"))return {DeveloperCodecCommand::CaptureFrame,true};
    if(words[0]=="lighting")return {DeveloperCodecCommand::Lighting,true};
    if(words[0]=="shadow")return {DeveloperCodecCommand::Shadow,true};
    if(words[0]=="environment")return {DeveloperCodecCommand::Environment,true};
    const bool known=words[0]=="help"||words[0]=="state"||words[0]=="soul"||words[0]=="battery"||words[0]=="enemies"||words[0]=="room"||words[0]=="colliders"||words[0]=="playtest"||words[0]=="capture"||words[0]=="lighting"||words[0]=="shadow"||words[0]=="environment";
    return {DeveloperCodecCommand::Invalid,known};
}

struct DeveloperCodecState {
    static constexpr int LineCount=9;
    bool open=false;
    bool showColliders=false;

    enum class Page : unsigned char {
        Root,
        Status,
        Rendering,
        Environment,
        Playtest
    };

    Page page=Page::Root;
    int selection=0;
    float menuScroll=0.0f;
    bool consoleFocused=false;

    bool actionFocus=true;
    int quickActionSelection=0;
    std::string input;
    std::array<std::string,LineCount> output{};
    std::array<std::string,LineCount> history{};
    int outputCount=0;
    int historyCount=0;
    int historyCursor=0;

    constexpr bool gameplayInputSuppressed() const noexcept{return open;}

    void write(const std::string& line){
        if(outputCount<LineCount)output[outputCount++]=line;
        else{for(int i=1;i<LineCount;++i)output[i-1]=output[i];output.back()=line;}
    }
    void remember(const std::string& line){
        if(line.empty())return;
        if(historyCount<LineCount)history[historyCount++]=line;
        else{for(int i=1;i<LineCount;++i)history[i-1]=history[i];history.back()=line;}
        historyCursor=historyCount;
    }
};
