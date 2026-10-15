#include "ModelData.hpp"

#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <limits>

namespace {
bool near(float a,float b){return std::fabs(a-b)<0.0001f;}
bool validAtScale(float scale){Vec3 n{};return triangle_geometry::faceNormal({0,0,0},{scale,0,0},{0,scale,0},n)&&near(n.z,1.0f);}
void require(bool condition,const char* message){if(condition)return;std::fprintf(stderr,"MODEL_VALIDATION_FAILED %s\n",message);std::exit(1);}
void validateNormalizedArchitecturalMesh(const char* path,std::size_t minimumVertices){
    StaticModelData model;
    require(model.load(path)&&model.valid(),"architectural mesh failed to load");
    require(model.vertices.size()/3>=minimumVertices,"architectural detail was unexpectedly flattened");
    float minimumY=std::numeric_limits<float>::infinity(),maximumY=-minimumY;
    int finiteNormals=0;
    for(std::size_t i=0;i+2<model.vertices.size();i+=3){
        minimumY=std::min(minimumY,model.vertices[i+1]);
        maximumY=std::max(maximumY,model.vertices[i+1]);
        const float lengthSquared=model.normals[i]*model.normals[i]+model.normals[i+1]*model.normals[i+1]+model.normals[i+2]*model.normals[i+2];
        if(std::isfinite(lengthSquared)&&lengthSquared>0.5f)++finiteNormals;
    }
    require(near(minimumY,0.0f)&&near(maximumY,1.0f),"architectural normalized bounds changed");
    require(finiteNormals>static_cast<int>(minimumVertices*0.80f),"architectural normals are invalid");
}
}

int main(){
    require(validAtScale(100.0f),"large normal scale");
    require(validAtScale(1.0f),"unit normal scale");
    require(validAtScale(0.16f),"small normal scale");
    require(validAtScale(0.000001f),"tiny normal scale");

    Vec3 normal{};
    require(triangle_geometry::faceNormal({0,0,0},{0,1,0},{1,0,0},normal)&&near(normal.z,-1.0f),"normal winding");
    require(!triangle_geometry::faceNormal({0,0,0},{0,0,0},{1,0,0},normal),"duplicate vertex rejection");
    require(!triangle_geometry::faceNormal({0,0,0},{1,0,0},{2,0,0},normal),"collinear rejection");
    require(!triangle_geometry::faceNormal({0,0,0},{1,0,0},{1,0.00001f,0},normal),"near-degenerate rejection");
    require(!triangle_geometry::faceNormal({0,0,0},{std::numeric_limits<float>::infinity(),0,0},{0,1,0},normal),"non-finite rejection");

    StaticModelData phone;
    require(phone.load(DB_PHONE_MODEL_PATH),"phone model failed to load");
    int valid=0,rejected=0;
    for(std::size_t i=0;i+8<phone.normals.size();i+=9){
        const float lengthSquared=phone.normals[i]*phone.normals[i]+phone.normals[i+1]*phone.normals[i+1]+phone.normals[i+2]*phone.normals[i+2];
        if(lengthSquared>0.5f)++valid;else ++rejected;
    }
    require(valid==3924,"phone valid normal count changed");
    require(rejected==16,"phone rejected normal count changed");
    validateNormalizedArchitecturalMesh(DB_CITY_WALL_WINDOW_MODEL_PATH,8000);
    validateNormalizedArchitecturalMesh(DB_CITY_WALL_DOOR_MODEL_PATH,6000);
    validateNormalizedArchitecturalMesh(DB_CITY_WALL_CORNER_MODEL_PATH,27000);
    validateNormalizedArchitecturalMesh(DB_CITY_CROWN_MODEL_PATH,2000);
    std::printf("MODEL_NORMAL_VALIDATION_OK valid=%d rejected=%d\n",valid,rejected);
    return 0;
}
