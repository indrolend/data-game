#pragma once
#include <sstream>
#include <string>
#include <vector>

enum class EnvironmentCommandKind : unsigned char { Invalid, Show, Catalog, Premise, Seed, Next, Reroll };
struct EnvironmentCommand { EnvironmentCommandKind kind=EnvironmentCommandKind::Invalid;int premise=0;int seed=0;int room=1; };

inline bool parseEnvironmentInteger(const std::string& text,int& value){try{std::size_t used=0;value=std::stoi(text,&used);return used==text.size();}catch(...){return false;}}
inline int environmentPremiseIndex(const std::string& name){
    static constexpr const char* names[]={"field-open","city-corridor","city-courtyard","city-canyon","city-skyline","sterile-corridor","sterile-chamber","coastal-shore"};
    for(int i=0;i<8;++i)if(name==names[i])return i;
    int index=-1;if(parseEnvironmentInteger(name,index)&&index>=0&&index<8)return index;return -1;
}
inline EnvironmentCommand parseEnvironmentCommand(const std::string& input){
    std::istringstream stream(input);std::vector<std::string> words;std::string word;while(stream>>word)words.push_back(word);EnvironmentCommand result;
    if(words.size()==2&&words[0]=="environment"&&words[1]=="show")result.kind=EnvironmentCommandKind::Show;
    else if(words.size()==2&&words[0]=="environment"&&words[1]=="catalog")result.kind=EnvironmentCommandKind::Catalog;
    else if(words.size()==3&&words[0]=="environment"&&words[1]=="premise"&&(result.premise=environmentPremiseIndex(words[2]))>=0)result.kind=EnvironmentCommandKind::Premise;
    else if((words.size()==3||words.size()==4)&&words[0]=="environment"&&words[1]=="seed"&&parseEnvironmentInteger(words[2],result.seed)&&(words.size()==3||parseEnvironmentInteger(words[3],result.room))&&result.room>=0)result.kind=EnvironmentCommandKind::Seed;
    else if(words.size()==2&&words[0]=="environment"&&words[1]=="next")result.kind=EnvironmentCommandKind::Next;
    else if(words.size()==2&&words[0]=="environment"&&words[1]=="reroll")result.kind=EnvironmentCommandKind::Reroll;
    return result;
}
