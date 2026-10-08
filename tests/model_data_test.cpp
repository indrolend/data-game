#include "ModelData.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

namespace {
bool near(float a,float b){return std::fabs(a-b)<0.0001f;}
bool validAtScale(float scale){Vec3 n{};return triangle_geometry::faceNormal({0,0,0},{scale,0,0},{0,scale,0},n)&&near(n.z,1.0f);}
}

int main(){
    assert(validAtScale(100.0f));
    assert(validAtScale(1.0f));
    assert(validAtScale(0.16f));
    assert(validAtScale(0.000001f));

    Vec3 normal{};
    assert(triangle_geometry::faceNormal({0,0,0},{0,1,0},{1,0,0},normal)&&near(normal.z,-1.0f));
    assert(!triangle_geometry::faceNormal({0,0,0},{0,0,0},{1,0,0},normal));
    assert(!triangle_geometry::faceNormal({0,0,0},{1,0,0},{2,0,0},normal));
    assert(!triangle_geometry::faceNormal({0,0,0},{1,0,0},{1,0.00001f,0},normal));
    assert(!triangle_geometry::faceNormal({0,0,0},{std::numeric_limits<float>::infinity(),0,0},{0,1,0},normal));

    StaticModelData phone;
    assert(phone.load(DB_PHONE_MODEL_PATH));
    int valid=0,rejected=0;
    for(std::size_t i=0;i+8<phone.normals.size();i+=9){
        const float lengthSquared=phone.normals[i]*phone.normals[i]+phone.normals[i+1]*phone.normals[i+1]+phone.normals[i+2]*phone.normals[i+2];
        if(lengthSquared>0.5f)++valid;else ++rejected;
    }
    assert(valid==3924);
    assert(rejected==16);
    std::printf("MODEL_NORMAL_VALIDATION_OK valid=%d rejected=%d\n",valid,rejected);
    return 0;
}
