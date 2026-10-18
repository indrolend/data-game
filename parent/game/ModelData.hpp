#pragma once

#include "TriangleNormal.hpp"

#include <cstdint>
#include <cmath>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

struct StaticModelBatch {
    std::uint32_t start=0,count=0;
    float color[4]{1,1,1,1};
};

struct StaticModelData {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<StaticModelBatch> batches;
    bool load(const std::string& path) {
        std::ifstream input(path,std::ios::binary|std::ios::ate);
        if(!input) return false;
        const auto size=input.tellg(); if(size<12) return false;
        input.seekg(0); std::vector<unsigned char> bytes(static_cast<std::size_t>(size));
        if(!input.read(reinterpret_cast<char*>(bytes.data()),size)) return false;
        if(std::memcmp(bytes.data(),"DBM1",4)!=0) return false;
        std::uint32_t vertexCount=0,batchCount=0; std::memcpy(&vertexCount,bytes.data()+4,4);std::memcpy(&batchCount,bytes.data()+8,4);
        const std::size_t expected=12u+static_cast<std::size_t>(vertexCount)*12u+static_cast<std::size_t>(batchCount)*24u;
        if(bytes.size()!=expected || vertexCount>1000000u || batchCount>1024u) return false;
        vertices.resize(static_cast<std::size_t>(vertexCount)*3u); std::memcpy(vertices.data(),bytes.data()+12,vertices.size()*sizeof(float));
        normals.assign(vertices.size(),0.0f);for(std::size_t i=0;i+8<vertices.size();i+=9){const Vec3 a{vertices[i],vertices[i+1],vertices[i+2]},b{vertices[i+3],vertices[i+4],vertices[i+5]},c{vertices[i+6],vertices[i+7],vertices[i+8]};Vec3 normal{};triangle_geometry::faceNormal(a,b,c,normal);for(int vertex=0;vertex<3;++vertex){normals[i+vertex*3]=normal.x;normals[i+vertex*3+1]=normal.y;normals[i+vertex*3+2]=normal.z;}}
        batches.resize(batchCount); std::size_t at=12+vertices.size()*sizeof(float);
        for(auto& batch:batches){std::memcpy(&batch.start,bytes.data()+at,4);std::memcpy(&batch.count,bytes.data()+at+4,4);std::memcpy(batch.color,bytes.data()+at+8,16);at+=24;if(batch.start+batch.count>vertexCount)return false;}
        return true;
    }
    bool valid() const { return !vertices.empty() && !batches.empty(); }
};
