#pragma once

#include "CoastalWater.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace coastal_water {

// A compact finite-difference shallow-water surface. Height and vertical
// velocity are integrated independently of the render mesh so graphics
// quality can change without changing the behavior of an impulse.
class Simulation {
public:
    static constexpr int Columns = 24;
    static constexpr int Rows = 48;
    static constexpr int CellCount = Columns * Rows;

    void reset() {
        height_.fill(0.0f);
        velocity_.fill(0.0f);
        nextVelocity_.fill(0.0f);
    }

    void disturb(float x, float localZ, float amount, float radius=0.72f) {
        if(!contains(x,localZ)||std::abs(amount)<0.000001f)return;
        const float radiusSq=radius*radius;
        for(int row=1;row<Rows-1;++row)for(int column=1;column<Columns-1;++column){
            const float cellX=ShoreX+(FarX-ShoreX)*static_cast<float>(column)/(Columns-1);
            const float cellZ=MinZ+(MaxZ-MinZ)*static_cast<float>(row)/(Rows-1);
            const float dx=cellX-x,dz=cellZ-localZ,distanceSq=dx*dx+dz*dz;
            if(distanceSq>=radiusSq)continue;
            const float falloff=1.0f-distanceSq/radiusSq;
            velocity_[index(column,row)]+=amount*falloff*falloff;
        }
    }

    void step(float dt) {
        dt=std::max(0.0f,std::min(1.0f/60.0f,dt));
        constexpr float propagation=7.4f;
        constexpr float damping=1.28f;
        for(int row=1;row<Rows-1;++row)for(int column=1;column<Columns-1;++column){
            const int center=index(column,row);
            const float laplacian=height_[index(column-1,row)]+height_[index(column+1,row)]
                +height_[index(column,row-1)]+height_[index(column,row+1)]-4.0f*height_[center];
            nextVelocity_[center]=(velocity_[center]+laplacian*propagation*dt)*std::exp(-damping*dt);
        }
        for(int row=1;row<Rows-1;++row)for(int column=1;column<Columns-1;++column){
            const int cell=index(column,row);
            velocity_[cell]=nextVelocity_[cell];
            height_[cell]=std::max(-0.13f,std::min(0.13f,height_[cell]+velocity_[cell]*dt));
            // Absorb energy at the sand and room seams instead of reflecting
            // perfectly rectangular waves back into the playable water.
            const int edge=std::min(std::min(column,Columns-1-column),std::min(row,Rows-1-row));
            if(edge<3){const float absorb=0.72f+0.09f*edge;height_[cell]*=absorb;velocity_[cell]*=absorb;}
        }
    }

    float sample(float x,float localZ) const {
        if(!contains(x,localZ))return 0.0f;
        const float gx=(x-ShoreX)/(FarX-ShoreX)*(Columns-1),gz=(localZ-MinZ)/(MaxZ-MinZ)*(Rows-1);
        const int x0=std::max(0,std::min(Columns-2,static_cast<int>(gx))),z0=std::max(0,std::min(Rows-2,static_cast<int>(gz)));
        const float tx=gx-x0,tz=gz-z0;
        const float a=height_[index(x0,z0)]*(1.0f-tx)+height_[index(x0+1,z0)]*tx;
        const float b=height_[index(x0,z0+1)]*(1.0f-tx)+height_[index(x0+1,z0+1)]*tx;
        return a*(1.0f-tz)+b*tz;
    }

    float energy() const {
        float result=0.0f;for(int i=0;i<CellCount;++i)result+=height_[i]*height_[i]+velocity_[i]*velocity_[i]*0.04f;return result;
    }

private:
    static constexpr int index(int column,int row){return row*Columns+column;}
    std::array<float,CellCount> height_{};
    std::array<float,CellCount> velocity_{};
    std::array<float,CellCount> nextVelocity_{};
};

} // namespace coastal_water
