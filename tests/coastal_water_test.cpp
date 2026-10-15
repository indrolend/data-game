#include "CoastalWater.hpp"
#include "CoastalWaterSimulation.hpp"

#include <cassert>
#include <cmath>

int main() {
    using namespace coastal_water;
    assert(!contains(ShoreX - 0.01f, 0.0f));
    assert(contains(ShoreX + 0.01f, 0.0f));
    assert(!contains(FarX + 0.01f, 0.0f));

    const float heightA=surfaceHeight(ShoreX+1.0f,2.0f,3.0f);
    const float heightB=surfaceHeight(ShoreX+1.0f,2.0f,3.0f);
    assert(std::isfinite(heightA));
    assert(std::abs(heightA-heightB)<0.000001f);
    assert(heightA>-0.10f&&heightA<0.15f);

    assert(immersion(ShoreX-1.0f,0.0f,0.0f,0.0f)==0.0f);
    const float shallow=immersion(ShoreX+0.1f,0.0f,0.0f,0.0f);
    const float deep=immersion(FarX-0.1f,0.0f,0.0f,0.0f);
    assert(shallow>0.0f);
    assert(deep>shallow);

    assert(wakeDisplacement(ShoreX+1.0f,0.5f,ShoreX+1.0f,0.0f,0.0f,1.0f)==0.0f);
    const float wake=wakeDisplacement(ShoreX+1.4f,0.0f,ShoreX+1.0f,0.0f,8.0f,1.0f);
    assert(std::isfinite(wake));
    assert(std::abs(wake)>0.00001f);

    Simulation simulation;
    const float sourceX=ShoreX+2.0f;
    simulation.disturb(sourceX,0.0f,-0.8f,0.55f);
    for(int step=0;step<24;++step)simulation.step(1.0f/120.0f);
    assert(simulation.energy()>0.000001f);
    assert(std::abs(simulation.sample(sourceX,0.0f))>0.00001f);
    assert(std::abs(simulation.sample(sourceX+0.75f,0.0f))>0.000001f);
    return 0;
}
