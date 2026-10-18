#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace field_grass_texture {

constexpr int Size=64;

inline std::uint32_t hash(int x,int y,std::uint32_t seed){
    std::uint32_t value=static_cast<std::uint32_t>(x)*374761393u
        +static_cast<std::uint32_t>(y)*668265263u+seed;
    value=(value^(value>>13u))*1274126177u;
    return value^(value>>16u);
}

inline float smooth(float value){return value*value*(3.0f-2.0f*value);}

inline float periodicNoise(int x,int y,int frequency,std::uint32_t seed){
    const float gx=static_cast<float>(x)*frequency/static_cast<float>(Size);
    const float gy=static_cast<float>(y)*frequency/static_cast<float>(Size);
    const int x0=static_cast<int>(std::floor(gx)),y0=static_cast<int>(std::floor(gy));
    const int x1=(x0+1)%frequency,y1=(y0+1)%frequency;
    const auto sample=[&](int sx,int sy){
        return static_cast<float>(hash((sx+frequency)%frequency,
            (sy+frequency)%frequency,seed)&65535u)/32767.5f-1.0f;
    };
    const float tx=smooth(gx-static_cast<float>(x0));
    const float ty=smooth(gy-static_cast<float>(y0));
    const float a=sample(x0,y0)+(sample(x1,y0)-sample(x0,y0))*tx;
    const float b=sample(x0,y1)+(sample(x1,y1)-sample(x0,y1))*tx;
    return a+(b-a)*ty;
}

inline std::array<unsigned char,Size*Size*3> pixels(){
    std::array<unsigned char,Size*Size*3> out{};
    for(int y=0;y<Size;++y)for(int x=0;x<Size;++x){
        const float broad=periodicNoise(x,y,2,0x91e10da5u);
        const float patch=periodicNoise(x,y,5,0x6d2b79f5u);
        const float fine=periodicNoise(x,y,17,0xa511e9b3u);
        const float direction=std::sin((static_cast<float>(x)*0.34f
            +static_cast<float>(y)*1.18f)*3.14159265f);
        const float blade=std::max(0.0f,direction-0.72f)
            *(0.35f+0.65f*std::max(0.0f,fine));
        const int red=static_cast<int>(50.0f+broad*8.0f+patch*5.0f+fine*2.5f);
        const int green=static_cast<int>(112.0f+broad*15.0f+patch*10.0f
            +fine*4.0f+blade*24.0f);
        const int blue=static_cast<int>(53.0f+broad*7.0f+patch*4.0f
            +fine*2.0f+blade*5.0f);
        const int index=(y*Size+x)*3;
        out[index]=static_cast<unsigned char>(std::clamp(red,0,255));
        out[index+1]=static_cast<unsigned char>(std::clamp(green,0,255));
        out[index+2]=static_cast<unsigned char>(std::clamp(blue,0,255));
    }
    return out;
}

} // namespace field_grass_texture
