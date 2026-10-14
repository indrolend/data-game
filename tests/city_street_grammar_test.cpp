#include "CityStreetGrammar.hpp"

#include <cassert>
#include <cmath>
#include <iostream>

int main(){
    const auto low=city_street::plan(room_environment::RoomForm::Corridor,12345u,0);
    const auto pretty=city_street::plan(room_environment::RoomForm::Corridor,12345u,2);
    assert(std::abs(low.roadHalfWidth-pretty.roadHalfWidth)<0.0001f);
    assert(std::abs(low.sidewalkWidth-pretty.sidewalkWidth)<0.0001f);
    assert(std::abs(low.intersectionCenter-pretty.intersectionCenter)<0.0001f);
    assert(low.lampPairs<pretty.lampPairs&&low.skylineBands<pretty.skylineBands);
    const auto a=city_street::skylineBlock(pretty,12345u,0,-1,2);
    const auto b=city_street::skylineBlock(pretty,12345u,0,-1,2);
    assert(a.x==b.x&&a.z==b.z&&a.height==b.height);
    assert(a.x< -15.0f&&a.height>5.0f&&a.width>2.0f);
    std::cout<<"CITY_STREET_GRAMMAR_OK\n";
}
