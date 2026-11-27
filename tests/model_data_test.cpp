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
void validateCityCorner(){
    StaticModelData model;
    require(model.load(DB_CITY_WALL_CORNER_MODEL_PATH),"City corner failed to load");
    require(model.vertices.size()==36 && model.batches.size()==1,
            "City corner must contain four triangles in one material batch");
    const auto& batch=model.batches.front();
    require(batch.start==0 && batch.count==12 && near(batch.color[0],0.49f) &&
            near(batch.color[1],0.475f) && near(batch.color[2],0.435f) &&
            near(batch.color[3],1.0f),"City corner material changed");

    int faceCounts[2]{};
    unsigned corners[2]{};
    for(std::size_t i=0;i<model.vertices.size();i+=9){
        const bool side=near(model.normals[i],-1.0f);
        require(side || near(model.normals[i+2],1.0f),"City corner face winding changed");
        const int face=side?0:1;
        ++faceCounts[face];
        for(std::size_t v=i;v<i+9;v+=3){
            const float x=model.vertices[v], y=model.vertices[v+1], z=model.vertices[v+2];
            const float across=side?z:x;
            require((side?near(x,-0.5f):near(z,0.5f)) &&
                    (near(across,-0.5f)||near(across,0.5f)) &&
                    (near(y,0.0f)||near(y,1.0f)),"City corner coverage changed");
            corners[face]|=1u<<((across>0.0f?1:0)+(y>0.5f?2:0));
        }
    }
    require(faceCounts[0]==2 && faceCounts[1]==2 &&
            corners[0]==15 && corners[1]==15,"City corner must cover both rectangular faces");
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
    validateCityCorner();
    validateNormalizedArchitecturalMesh(DB_CITY_CROWN_MODEL_PATH,2000);
    std::printf("MODEL_NORMAL_VALIDATION_OK valid=%d rejected=%d\n",valid,rejected);
    return 0;
}
