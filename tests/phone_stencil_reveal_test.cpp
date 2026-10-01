#include "PhoneStencilReveal.hpp"
#include <cassert>
#include <cstdio>

int main(){
    const auto seed=phone_stencil::textSeed("SETTINGS");
    assert(seed==phone_stencil::textSeed("SETTINGS"));
    assert(seed!=phone_stencil::textSeed("CONTROLS"));
    assert(phone_stencil::appearanceAge(-1.0f)==0.0f);
    assert(phone_stencil::appearanceAge(2.0f)==0.24f);
    const float earlyFirst=phone_stencil::glyphReveal(0.06f,0,8,true);
    const float earlyLast=phone_stencil::glyphReveal(0.06f,7,8,true);
    assert(earlyFirst>earlyLast);
    assert(phone_stencil::glyphReveal(0.24f,0,8,true)==1.0f);
    assert(phone_stencil::glyphReveal(0.24f,7,8,true)==1.0f);
    const float reverseFirst=phone_stencil::glyphReveal(0.06f,0,8,false);
    const float reverseLast=phone_stencil::glyphReveal(0.06f,7,8,false);
    assert(reverseLast>reverseFirst);
    std::puts("PHONE_STENCIL_REVEAL_OK deterministic=DIRECTIONAL bounded=YES");
}
