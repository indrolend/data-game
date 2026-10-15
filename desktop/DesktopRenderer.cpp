#include "DesktopRenderer.hpp"
#include "HumanVisual.hpp"
#include "BitmapFont.hpp"
#include "RoomEnvironment.hpp"
#include "RoomLighting.hpp"
#include "PhoneDisplayLayout.hpp"
#include "PhoneStencilReveal.hpp"
#include "GameplayPhoneModel.hpp"
#include "CityFacadeGrammar.hpp"
#include "CityStreetGrammar.hpp"
#include "RenderContracts.hpp"
#include "MaterialResponse.hpp"
#include "FieldGrassTexture.hpp"
#include "CitySurfaceTexture.hpp"
#include "FacetedRock.hpp"
#include "CoastalWater.hpp"
#include "CoastalWaterSimulation.hpp"
#include "MarkerPillarGeometry.hpp"
#include "world/RoomGeometry.hpp"
#include "gameplay/MaterialUnlocks.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace {
float displayedFps=60.0f;
int fpsFrames=0;
auto fpsWindowStart=std::chrono::steady_clock::now();
constexpr float ROOM_WIDTH = world::RoomWidth;
constexpr float ROOM_DEPTH = world::RoomDepth;
constexpr int ROOM_VISUAL_HORIZON = 2;
constexpr float ROOM_WALL_HEIGHT = world::RoomWallHeight;
constexpr float PI = 3.14159265358979323846f;
constexpr float SHADOW_PROJECTION_SCALE = 0.5f;
constexpr float PHONE_DISPLAY_INSET_X = 0.94f;
constexpr float PHONE_DISPLAY_INSET_Y = 0.96f;
constexpr float PHONE_DISPLAY_PLANE_OFFSET = 0.00115f;

struct MenuFontAtlas {
    std::vector<unsigned char> bytes;
    stbtt_fontinfo info{};
    bool cpuReady = false;

    bool load(const std::filesystem::path& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;
        bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        if (bytes.empty()) return false;
        cpuReady = stbtt_InitFont(&info, bytes.data(), stbtt_GetFontOffsetForIndex(bytes.data(), 0)) != 0;
        return cpuReady;
    }
};

MenuFontAtlas menuRegularFont;
MenuFontAtlas menuSemiboldFont;

void hashPhoneDisplayValue(std::uint64_t& hash, std::uint64_t value) {
    hash ^= value;
    hash *= 1099511628211ull;
}

void hashPhoneDisplayString(std::uint64_t& hash, const std::string& value) {
    for (unsigned char c : value) hashPhoneDisplayValue(hash, c);
    hashPhoneDisplayValue(hash, 0xffu);
}

void hashPhoneDisplayFloat(std::uint64_t& hash, float value, float scale) {
    hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(std::lround(value * scale)));
}

Vec3 gradedSceneColor(float r,float g,float b) {
    const float luma=r*0.2126f+g*0.7152f+b*0.0722f;
    r=luma+(r-luma)*1.10f;g=luma+(g-luma)*1.10f;b=luma+(b-luma)*1.10f;
    return {clampf((r-0.5f)*1.06f+0.5f,0.0f,1.0f),clampf((g-0.5f)*1.06f+0.5f,0.0f,1.0f),clampf((b-0.5f)*1.06f+0.5f,0.0f,1.0f)};
}
Vec3 mix3(const Vec3& a,const Vec3& b,float t){const float u=clampf(t,0.0f,1.0f);return {a.x+(b.x-a.x)*u,a.y+(b.y-a.y)*u,a.z+(b.z-a.z)*u};}
void gradedColor(float r,float g,float b,float a=1.0f){const Vec3 color=gradedSceneColor(r,g,b);glColor4f(color.x,color.y,color.z,a);}

void applyMaterial(const render_contract::MaterialDefinition& material){
    const GLfloat specular[]={material.specular.r,material.specular.g,material.specular.b,1.0f};
    const GLfloat emission[]={material.emission.r,material.emission.g,material.emission.b,1.0f};
    if(material.shading==render_contract::ShadingModel::Unlit)glDisable(GL_LIGHTING);else glEnable(GL_LIGHTING);
    if(material.fog)glEnable(GL_FOG);else glDisable(GL_FOG);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,specular);
    glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,material.shininess);
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,emission);
    gradedColor(material.baseColor.r,material.baseColor.g,material.baseColor.b,material.opacity);
}

void applyLight(GLenum slot,const room_lighting::ResolvedLight& light){
    if(!light.enabled){glDisable(slot);return;}
    const GLfloat diffuse[]={light.diffuse.r,light.diffuse.g,light.diffuse.b,1.0f};
    const GLfloat position[]={light.position.x,light.position.y,light.position.z,light.directional?0.0f:1.0f};
    glEnable(slot);glLightfv(slot,GL_DIFFUSE,diffuse);glLightfv(slot,GL_POSITION,position);
    glLightf(slot,GL_CONSTANT_ATTENUATION,light.constantAttenuation);
    glLightf(slot,GL_LINEAR_ATTENUATION,light.linearAttenuation);
    glLightf(slot,GL_QUADRATIC_ATTENUATION,light.quadraticAttenuation);
}

class ScopedMaterial {
public:
    explicit ScopedMaterial(const render_contract::MaterialDefinition& material,bool active=true):active_(active){if(active_)applyMaterial(material);}
    ~ScopedMaterial(){if(active_)applyMaterial(semanticMaterial(SemanticSurface::Default));}
    ScopedMaterial(const ScopedMaterial&)=delete;
    ScopedMaterial& operator=(const ScopedMaterial&)=delete;
private:
    bool active_=false;
};

Vec3 cross3(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
void perspective(float fovyDegrees, float aspect, float nearPlane, float farPlane) {
    const float top = nearPlane * std::tan(fovyDegrees * PI / 360.0f);
    const float right = top * aspect;
    glFrustum(-right, right, -top, top, nearPlane, farPlane);
}
void lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    const Vec3 forward = normalized(center - eye);
    const Vec3 side = normalized(cross3(forward, up));
    const Vec3 correctedUp = cross3(side, forward);
    const float matrix[16] = {
        side.x, correctedUp.x, -forward.x, 0,
        side.y, correctedUp.y, -forward.y, 0,
        side.z, correctedUp.z, -forward.z, 0,
        -dot3(side, eye), -dot3(correctedUp, eye), dot3(forward, eye), 1
    };
    glMultMatrixf(matrix);
}
void cube() {
    glBegin(GL_QUADS);
    glNormal3f(0,0,1);
    glVertex3f(-.5f,-.5f,.5f); glVertex3f(.5f,-.5f,.5f); glVertex3f(.5f,.5f,.5f); glVertex3f(-.5f,.5f,.5f);
    glNormal3f(0,0,-1);
    glVertex3f(.5f,-.5f,-.5f); glVertex3f(-.5f,-.5f,-.5f); glVertex3f(-.5f,.5f,-.5f); glVertex3f(.5f,.5f,-.5f);
    glNormal3f(-1,0,0);
    glVertex3f(-.5f,-.5f,-.5f); glVertex3f(-.5f,-.5f,.5f); glVertex3f(-.5f,.5f,.5f); glVertex3f(-.5f,.5f,-.5f);
    glNormal3f(1,0,0);
    glVertex3f(.5f,-.5f,.5f); glVertex3f(.5f,-.5f,-.5f); glVertex3f(.5f,.5f,-.5f); glVertex3f(.5f,.5f,.5f);
    glNormal3f(0,1,0);
    glVertex3f(-.5f,.5f,.5f); glVertex3f(.5f,.5f,.5f); glVertex3f(.5f,.5f,-.5f); glVertex3f(-.5f,.5f,-.5f);
    glNormal3f(0,-1,0);
    glVertex3f(-.5f,-.5f,-.5f); glVertex3f(.5f,-.5f,-.5f); glVertex3f(.5f,-.5f,.5f); glVertex3f(-.5f,-.5f,.5f);
    glEnd();
}

float shadowReceiverHeight(const GameState& state,float worldX,float worldZ,float radius){
    const float localZ=worldZ-std::floor((worldZ+ROOM_DEPTH*0.5f)/ROOM_DEPTH)*ROOM_DEPTH;
    float height=0.0f;
    for(int i=0;i<state.debug.colliderCount;++i){const RoomCollider& c=state.roomColliders[i];if(worldX>c.minX-radius&&worldX<c.maxX+radius&&localZ>c.minZ-radius&&localZ<c.maxZ+radius)height=std::max(height,c.topY);}
    for(int i=0;i<state.slopeSupportCount;++i){const auto sample=sampleSlopeSupport(state.slopeSupports[i],worldX,localZ);if(sample.inside&&sample.classification!=SupportClassification::Steep)height=std::max(height,sample.height);}
    for(int i=0;i<state.rockSupportCount;++i){const auto sample=faceted_rock::sampleSupportFootprint(state.rockSupports[i],worldX,localZ,radius);if(sample.inside)height=std::max(height,sample.height);}
    return height;
}

void drawGroundShadow(const Vec3& caster, float halfWidth, float halfDepth, float height, float alpha,float receiverY=0.0f) {
    constexpr int segments = 8;
    const Vec3& sunDirection=render_contract::DesktopSceneLighting.sun.direction;
    const float sunXOverY=sunDirection.x/sunDirection.y;
    const float sunZOverY=sunDirection.z/sunDirection.y;
    const Vec3 center{
        caster.x-height*SHADOW_PROJECTION_SCALE*sunXOverY,
        receiverY+0.012f,
        caster.z-height*SHADOW_PROJECTION_SCALE*sunZOverY
    };
    const float stretch=height*0.16f;
    const float angle=std::atan2(-sunXOverY,-sunZOverY);
    const float c=std::cos(angle),s=std::sin(angle);
    glColor4f(0.012f,0.018f,0.022f,alpha);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(center.x,center.y,center.z);
    for(int i=0;i<=segments;++i){
        const float a=2.0f*PI*static_cast<float>(i)/static_cast<float>(segments);
        const float localX=std::cos(a)*(halfWidth+stretch*0.35f);
        const float localZ=std::sin(a)*(halfDepth+stretch);
        glVertex3f(center.x+localX*c-localZ*s,center.y,center.z+localX*s+localZ*c);
    }
    glEnd();
}

void roundedEllipsoid(const Vec3& p, const Vec3& scale, float pitch, float yaw, float roll, float r, float g, float b) {
    constexpr int segments = 7;
    constexpr int rings = 5;
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    glRotatef(yaw * 180.0f / PI, 0, 1, 0);
    glRotatef(pitch * 180.0f / PI, 1, 0, 0);
    glRotatef(roll * 180.0f / PI, 0, 0, 1);
    glScalef(scale.x, scale.y, scale.z);
    gradedColor(r,g,b);
    auto vertex = [](int ring, int segment) {
        const float v = static_cast<float>(ring) / static_cast<float>(rings);
        const float phi = -PI * 0.5f + v * PI;
        const float u = static_cast<float>(segment) / static_cast<float>(segments);
        const float theta = u * PI * 2.0f;
        const float cp = std::cos(phi);
        const float x=std::cos(theta)*cp, y=std::sin(phi), z=std::sin(theta)*cp;
        glNormal3f(x,y,z); glVertex3f(x*0.5f,y*0.5f,z*0.5f);
    };
    glBegin(GL_TRIANGLES);
    for (int ring = 0; ring < rings; ++ring) {
        for (int seg = 0; seg < segments; ++seg) {
            const int nextSeg = (seg + 1) % segments;
            vertex(ring, seg); vertex(ring + 1, seg); vertex(ring + 1, nextSeg);
            vertex(ring, seg); vertex(ring + 1, nextSeg); vertex(ring, nextSeg);
        }
    }
    glEnd();
    glPopMatrix();
}

void drawProceduralHumanDesktop(const TargetState& target, float time, float r, float g, float b) {
    const HumanVisualSpec& spec = HUMAN_VISUAL_SPEC;
    const bool aliveHuman = !target.slurpable;
    const HumanBodyPresentation body=target.humanBodyPresentation();
    const HumanVisualPose pose = makeHumanVisualPose(target.visualYaw, target.scale, time, target.visualReaction, aliveHuman, body);
    if (pose.scale <= 0.001f) return;
    const float s = pose.scale;
    const float collapseScale = std::max(0.18f, 1.0f - pose.collapse * 0.62f);
    const Vec3 root = target.pos + Vec3{0.0f, spec.rootGroundOffset + pose.rootBob, 0.0f};
    const float yaw = pose.yaw;
    const Vec3 forward{-std::sin(yaw),0,-std::cos(yaw)};
    const Vec3 right{std::cos(yaw),0,-std::sin(yaw)};
    const float footY=0.03f*s;
    const float shinY=footY+spec.footHeight*s*0.5f+spec.shinLength*s*0.5f;
    const float thighY=footY+spec.footHeight*s+spec.shinLength*s+spec.thighLength*s*0.5f;
    const float pelvisY=footY+spec.footHeight*s+spec.shinLength*s+spec.thighLength*s+spec.pelvisHeight*s*0.5f;
    const float torsoY=pelvisY+(spec.pelvisHeight+spec.torsoHeight)*s*0.5f;
    const float headY=spec.totalHeight*s-spec.headRadius*s;
    const float armY=torsoY+spec.torsoHeight*s*0.18f;
    roundedEllipsoid(root+Vec3{0,pelvisY*collapseScale,0},{spec.pelvisWidth*s,spec.pelvisHeight*s*collapseScale,spec.pelvisDepth*s},0,yaw,0,r,g,b);
    roundedEllipsoid(root+Vec3{0,torsoY*collapseScale,0}+forward*((pose.hitLean + pose.vacuumLean * 0.06f)*s),{spec.torsoWidth*s,spec.torsoHeight*s*collapseScale,spec.torsoDepth*s},pose.torsoPitch,yaw,pose.torsoRoll,r,g,b);
    roundedEllipsoid(root+Vec3{0,headY,0}+forward*(pose.headPitch*0.03f),{spec.headRadius*2*s,spec.headRadius*2*s,spec.headRadius*2*s},pose.headPitch,yaw,0,r,g,b);
    for (int side : {-1,1}) {
        const float armSwing=side<0?pose.leftArmSwing:pose.rightArmSwing;
        const float legSwing=side<0?pose.leftLegSwing:pose.rightLegSwing;
        const Vec3 shoulder=root+right*(side*spec.shoulderWidth*0.5f*s)+Vec3{0,armY,0};
        roundedEllipsoid(shoulder+forward*(armSwing*0.06f*s)+Vec3{0,-spec.upperArmLength*0.5f*s*collapseScale,0},{0.055f*s,spec.upperArmLength*s*collapseScale,0.065f*s},armSwing,yaw,0,r,g,b);
        roundedEllipsoid(shoulder+forward*(armSwing*0.11f*s)+Vec3{0,-(spec.upperArmLength+spec.forearmLength*0.5f)*s*collapseScale,0},{0.052f*s,spec.forearmLength*s*collapseScale,0.060f*s},armSwing*0.7f,yaw,0,r,g,b);
        roundedEllipsoid(shoulder+forward*(armSwing*0.14f*s)+Vec3{0,-(spec.upperArmLength+spec.forearmLength)*s,0},{spec.handSize*s,spec.handSize*s,spec.handSize*0.75f*s},0,yaw,0,r,g,b);
        const Vec3 hip=root+right*(side*spec.pelvisWidth*0.28f*s);
        Vec3 thighCenter=hip+forward*(legSwing*0.05f*s)+Vec3{0,thighY*collapseScale,0};
        Vec3 shinCenter=hip-forward*(legSwing*0.05f*s)+Vec3{0,shinY*collapseScale,0};
        Vec3 footCenter=hip+forward*(spec.footLength*0.25f*s+legSwing*0.04f*s)+Vec3{0,footY,0};
        if(body.authoritative()){
            const Vec3 local=side<0?body.leftFootTarget():body.rightFootTarget();
            footCenter=target.pos+(right*local.x-forward*local.z+Vec3{0,local.y,0})*s
                +Vec3{0,spec.footHeight*s*0.5f,0};
            const Vec3 hipJoint=hip+Vec3{0,(spec.footHeight+spec.shinLength+spec.thighLength)*s*collapseScale,0};
            const Vec3 knee=(hipJoint+footCenter)*0.5f+forward*(0.055f*s);
            thighCenter=(hipJoint+knee)*0.5f;shinCenter=(knee+footCenter)*0.5f;
        }
        roundedEllipsoid(thighCenter,{0.075f*s,spec.thighLength*s*collapseScale,0.080f*s},legSwing,yaw,0,r,g,b);
        roundedEllipsoid(shinCenter,{0.070f*s,spec.shinLength*s*collapseScale,0.075f*s},-legSwing*0.65f,yaw,0,r,g,b);
        roundedEllipsoid(footCenter,{0.075f*s,spec.footHeight*s,spec.footLength*s},0,yaw,0,r,g,b);
    }
}
Quat quaternionFromEulerXYZ(float x,float y,float z) {
    const float c1=std::cos(x*0.5f),c2=std::cos(y*0.5f),c3=std::cos(z*0.5f),s1=std::sin(x*0.5f),s2=std::sin(y*0.5f),s3=std::sin(z*0.5f);
    return {s1*c2*c3+c1*s2*s3,c1*s2*c3-s1*c2*s3,c1*c2*s3+s1*s2*c3,c1*c2*c3-s1*s2*s3};
}

unsigned int compileStaticModel(const StaticModelData& model, bool shadow = false, bool inheritColor = false) {
    if (!model.valid()) return 0;
    const GLuint list=glGenLists(1);
    if (!list) return 0;
    glNewList(list,GL_COMPILE);
    for(const StaticModelBatch& batch:model.batches) {
        // Shadow display lists are drawn inside one renderer-owned blended pass.
        // Do not let source-material alpha toggle that pass off partway through
        // a multi-batch model, which would make subsequent shadow batches opaque.
        const bool translucent=!shadow&&batch.color[3]<0.995f;if(translucent){glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);}
        if(shadow) glColor4f(0.012f,0.018f,0.022f,0.28f); else if(!inheritColor) gradedColor(batch.color[0],batch.color[1],batch.color[2],batch.color[3]);
        glBegin(GL_TRIANGLES);
        for(std::uint32_t i=batch.start;i+2<batch.start+batch.count;i+=3) {
            const std::size_t a=static_cast<std::size_t>(i)*3u;
            const std::size_t b=a+3u;
            const std::size_t c=a+6u;
            const Vec3 va{model.vertices[a],model.vertices[a+1],model.vertices[a+2]};
            const Vec3 vb{model.vertices[b],model.vertices[b+1],model.vertices[b+2]};
            const Vec3 vc{model.vertices[c],model.vertices[c+1],model.vertices[c+2]};
            Vec3 normal{};
            const bool storedNormal=model.normals.size()==model.vertices.size();
            if(storedNormal)normal={model.normals[a],model.normals[a+1],model.normals[a+2]};
            const bool validStoredNormal=storedNormal&&std::isfinite(normal.x)&&std::isfinite(normal.y)&&std::isfinite(normal.z)&&lengthSq(normal)>0.5f;
            if(validStoredNormal)normal=normal*(1.0f/std::sqrt(lengthSq(normal)));
            else triangle_geometry::faceNormal(va,vb,vc,normal);
            glNormal3f(normal.x,normal.y,normal.z);
            glVertex3f(va.x,va.y,va.z); glVertex3f(vb.x,vb.y,vb.z); glVertex3f(vc.x,vc.y,vc.z);
        }
        glEnd();
        if(translucent){glDepthMask(GL_TRUE);glDisable(GL_BLEND);}
    }
    glEndList();
    return list;
}

}

void DesktopRenderer::setAssetRoot(const std::filesystem::path& root) {
    tvGifWall_.load(root.parent_path()/"tv-gifs");
    const std::filesystem::path fontRoot = root.parent_path() / "fonts";
    const bool menuRegularLoaded = menuRegularFont.load(fontRoot / "SourceSans3-Regular.ttf");
    const bool menuSemiboldLoaded = menuSemiboldFont.load(fontRoot / "SourceSans3-Semibold.ttf");
    bool birdSpriteLoaded=false;
    {
        constexpr int width=1024,height=256;
        std::ifstream input(root/"visual-humor-v-birds.rgba",std::ios::binary);
        std::vector<unsigned char> pixels(std::istreambuf_iterator<char>(input),{});
        if(pixels.size()==static_cast<std::size_t>(width*height*4)){
            glGenTextures(1,&birdSpriteTexture_);glBindTexture(GL_TEXTURE_2D,birdSpriteTexture_);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
            birdSpriteLoaded=true;
        }
    }
    const auto loadPhotoMaterial=[&](const std::filesystem::path& path,unsigned int& texture){
        constexpr int size=512;
        std::ifstream input(path,std::ios::binary);
        std::vector<unsigned char> pixels(std::istreambuf_iterator<char>(input),{});
        if(pixels.size()!=static_cast<std::size_t>(size*size*3))return false;
        glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,size,size,0,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
        return true;
    };
    const auto materialRoot=root/"materials";
    bool foliageLoaded=false;
    {
        constexpr int size=512;
        std::ifstream input(materialRoot/"cc0-fir-foliage.rgba",std::ios::binary);
        std::vector<unsigned char> pixels(std::istreambuf_iterator<char>(input),{});
        if(pixels.size()==static_cast<std::size_t>(size*size*4)){
            glGenTextures(1,&foliageTexture_);glBindTexture(GL_TEXTURE_2D,foliageTexture_);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,size,size,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
            foliageLoaded=true;
        }
    }
    const bool grassPhotoLoaded=loadPhotoMaterial(materialRoot/"photo-grass.rgb",fieldGrassTexture_);
    const bool barkPhotoLoaded=loadPhotoMaterial(materialRoot/"photo-bark.rgb",barkTexture_);
    const bool stonePhotoLoaded=loadPhotoMaterial(materialRoot/"photo-stone-runtime.rgb",stoneTexture_);
    const bool metalPhotoLoaded=loadPhotoMaterial(materialRoot/"photo-metal-runtime.rgb",metalTexture_);
    const bool sandPhotoLoaded=loadPhotoMaterial(materialRoot/"photo-sand.rgb",sandTexture_);
    const bool sterileTileLoaded=loadPhotoMaterial(materialRoot/"photo-sterile-tile.rgb",sterileTileTexture_);
    StaticModelData phone;
    StaticModelData commonTree,twistedTree,deadTree,pineTree,cityWindowBay,cityWindowBaySmall,cityDoorBay,cityDoorWindowBay,cityCornice;
    const bool phoneLoaded=phone.load((root/"phone.dbmesh").string());
    const bool humanLoaded=humanModel_.load((root/"human.dbhuman").string());
    phoneModelList_=phoneLoaded?compileStaticModel(phone):0; phoneShadowList_=phoneLoaded?compileStaticModel(phone,true):0;
    commonTreeModelList_=commonTree.load((root/"environment"/"common-tree.dbmesh").string())?compileStaticModel(commonTree):0;
    twistedTreeModelList_=twistedTree.load((root/"environment"/"twisted-tree.dbmesh").string())?compileStaticModel(twistedTree):0;
    deadTreeModelList_=deadTree.load((root/"environment"/"dead-tree.dbmesh").string())?compileStaticModel(deadTree):0;
    pineTreeModelList_=pineTree.load((root/"environment"/"pine-tree.dbmesh").string())?compileStaticModel(pineTree):0;
    cityWindowBayModelList_=cityWindowBay.load((root/"environment"/"city"/"window-bay.dbmesh").string())?compileStaticModel(cityWindowBay):0;
    cityWindowBaySmallModelList_=cityWindowBaySmall.load((root/"environment"/"city"/"window-bay-small.dbmesh").string())?compileStaticModel(cityWindowBaySmall):0;
    cityDoorBayModelList_=cityDoorBay.load((root/"environment"/"city"/"door-bay.dbmesh").string())?compileStaticModel(cityDoorBay):0;
    cityDoorWindowBayModelList_=cityDoorWindowBay.load((root/"environment"/"city"/"door-window-bay.dbmesh").string())?compileStaticModel(cityDoorWindowBay):0;
    cityCorniceModelList_=cityCornice.load((root/"environment"/"city"/"cornice.dbmesh").string())?compileStaticModel(cityCornice,false,true):0;
    std::printf("Models: phone=%s flower=prismatic human=%s trees=%s/%s/%s/%s cityModules=%s/%s/%s/%s/%s menuFont=%s/%s birds=%s foliage=%s photos=%s/%s/%s/%s/%s/%s\n",phoneModelList_?"loaded":"fallback",humanLoaded?"loaded":"fallback",commonTreeModelList_?"common":"off",twistedTreeModelList_?"twisted":"off",deadTreeModelList_?"dead":"off",pineTreeModelList_?"pine":"off",cityWindowBayModelList_?"window":"off",cityWindowBaySmallModelList_?"small-window":"off",cityDoorBayModelList_?"door":"off",cityDoorWindowBayModelList_?"door-window":"off",cityCorniceModelList_?"cornice":"off",menuRegularLoaded?"regular":"bitmap",menuSemiboldLoaded?"semibold":"bitmap",birdSpriteLoaded?"v-atlas":"off",foliageLoaded?"cc0-fir":"fallback",grassPhotoLoaded?"grass":"procedural",barkPhotoLoaded?"bark":"off",stonePhotoLoaded?"stone":"off",metalPhotoLoaded?"metal":"off",sandPhotoLoaded?"sand":"off",sterileTileLoaded?"sterile":"off");
}

void DesktopRenderer::resize(int width, int height) {
    const int nextWidth=std::max(1,width),nextHeight=std::max(1,height);
    if(nextWidth!=width_||nextHeight!=height_)datamoshFrameReady_=false;
    width_=nextWidth;height_=nextHeight;glViewport(0,0,width_,height_);
}

void DesktopRenderer::setHudVisible(bool visible) {
    hudVisible_ = visible;
}

void DesktopRenderer::setAtmosphereProfile(render_contract::AtmosphereProfile profile){lightingControl_.reference=profile;}
void DesktopRenderer::setLightingControl(const render_contract::RuntimeLightingControl& control){lightingControl_=control;}
render_contract::RuntimeLightingControl& DesktopRenderer::lightingControl(){return lightingControl_;}
const render_contract::RuntimeLightingControl& DesktopRenderer::lightingControl() const{return lightingControl_;}
render_contract::SceneAtmosphere DesktopRenderer::resolvedAtmosphere(const GameState& state) const{
    const auto atmosphere=render_contract::resolveSceneAtmosphere(lightingControl_,{state.time,state.roomIndex,state.vacuum.power*0.62f+state.energy.dischargePositionAmount});
    const auto setting=room_environment::roomPlan(state.roomSeed,state.roomIndex).setting;
    return room_lighting::settingAtmosphere(atmosphere,setting,lightingControl_.reference,lightingControl_.overrideMask);
}

void DesktopRenderer::drawBox(const Vec3& p, const Vec3& s, float pitch, float yaw, float roll, float r, float g, float b, float a) {
    glPushMatrix();
    glTranslatef(p.x, p.y, p.z);
    glRotatef(yaw * 180.0f / PI, 0, 1, 0);
    glRotatef(pitch * 180.0f / PI, 1, 0, 0);
    glRotatef(roll * 180.0f / PI, 0, 0, 1);
    glScalef(s.x, s.y, s.z); gradedColor(r,g,b,a); cube(); glPopMatrix();
}

void DesktopRenderer::drawTexturedBox(unsigned int texture,const Vec3& p,const Vec3& s,float pitch,float yaw,float roll,const VisualColor& tint,float textureWorldScale,float texturePhase){
    if(!texture){drawBox(p,s,pitch,yaw,roll,tint.r,tint.g,tint.b);return;}
    const float unit=1.0f/std::max(0.05f,textureWorldScale),uX=s.x*unit,uY=s.y*unit,uZ=s.z*unit;
    glPushMatrix();glTranslatef(p.x,p.y,p.z);glRotatef(yaw*180.0f/PI,0,1,0);glRotatef(pitch*180.0f/PI,1,0,0);glRotatef(roll*180.0f/PI,0,0,1);glScalef(s.x,s.y,s.z);
    glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture);
    glMatrixMode(GL_TEXTURE);glPushMatrix();glLoadIdentity();glTranslatef(texturePhase,std::fmod(texturePhase*1.618f,1.0f),0.0f);glMatrixMode(GL_MODELVIEW);
    gradedColor(tint.r,tint.g,tint.b);glBegin(GL_QUADS);
    const auto face=[](float nx,float ny,float nz,float ua,float va,float ub,float vb,float uc,float vc,float ud,float vd,
        float ax,float ay,float az,float bx,float by,float bz,float cx,float cy,float cz,float dx,float dy,float dz){
        glNormal3f(nx,ny,nz);glTexCoord2f(ua,va);glVertex3f(ax,ay,az);glTexCoord2f(ub,vb);glVertex3f(bx,by,bz);glTexCoord2f(uc,vc);glVertex3f(cx,cy,cz);glTexCoord2f(ud,vd);glVertex3f(dx,dy,dz);
    };
    face(0,0,1, 0,0,uX,0,uX,uY,0,uY, -.5f,-.5f,.5f,.5f,-.5f,.5f,.5f,.5f,.5f,-.5f,.5f,.5f);
    face(0,0,-1,uX,0,0,0,0,uY,uX,uY, .5f,-.5f,-.5f,-.5f,-.5f,-.5f,-.5f,.5f,-.5f,.5f,.5f,-.5f);
    face(1,0,0, 0,0,uZ,0,uZ,uY,0,uY, .5f,-.5f,.5f,.5f,-.5f,-.5f,.5f,.5f,-.5f,.5f,.5f,.5f);
    face(-1,0,0,uZ,0,0,0,0,uY,uZ,uY, -.5f,-.5f,-.5f,-.5f,-.5f,.5f,-.5f,.5f,.5f,-.5f,.5f,-.5f);
    face(0,1,0, 0,0,uX,0,uX,uZ,0,uZ, -.5f,.5f,.5f,.5f,.5f,.5f,.5f,.5f,-.5f,-.5f,.5f,-.5f);
    face(0,-1,0,0,uZ,uX,uZ,uX,0,0,0, -.5f,-.5f,-.5f,.5f,-.5f,-.5f,.5f,-.5f,.5f,-.5f,-.5f,.5f);
    glEnd();glMatrixMode(GL_TEXTURE);glPopMatrix();glMatrixMode(GL_MODELVIEW);glDisable(GL_TEXTURE_2D);glPopMatrix();
}

void fxRibbon(const Vec3& p,const Quat& q,const Vec3& scale,float start,float sweep,int segments,float inner,float outer,float r,float g,float b,float a){
    const float matrix[16]={1-2*(q.y*q.y+q.z*q.z),2*(q.x*q.y+q.z*q.w),2*(q.x*q.z-q.y*q.w),0,2*(q.x*q.y-q.z*q.w),1-2*(q.x*q.x+q.z*q.z),2*(q.y*q.z+q.x*q.w),0,2*(q.x*q.z+q.y*q.w),2*(q.y*q.z-q.x*q.w),1-2*(q.x*q.x+q.y*q.y),0,0,0,0,1};
    glPushMatrix(); glTranslatef(p.x,p.y,p.z); glMultMatrixf(matrix); glScalef(scale.x,scale.y,scale.z); gradedColor(r,g,b,a);
    glBegin(GL_TRIANGLE_STRIP); for(int i=0;i<=segments;++i){const float angle=start+sweep*static_cast<float>(i)/segments; const float c=std::cos(angle),s=std::sin(angle); glVertex3f(c*outer,s*outer,0); glVertex3f(c*inner,s*inner,0);} glEnd(); glPopMatrix();
}

void fxStreak(const Vec3& p,const Quat& q,float length,float width,float r,float g,float b,float a){
    constexpr int segments=12; const float matrix[16]={1-2*(q.y*q.y+q.z*q.z),2*(q.x*q.y+q.z*q.w),2*(q.x*q.z-q.y*q.w),0,2*(q.x*q.y-q.z*q.w),1-2*(q.x*q.x+q.z*q.z),2*(q.y*q.z+q.x*q.w),0,2*(q.x*q.z+q.y*q.w),2*(q.y*q.z-q.x*q.w),1-2*(q.x*q.x+q.y*q.y),0,0,0,0,1};
    glPushMatrix(); glTranslatef(p.x,p.y,p.z); glMultMatrixf(matrix); gradedColor(r,g,b,a); glBegin(GL_TRIANGLE_STRIP);
    for(int i=0;i<=segments;++i){const float angle=i*PI*2.0f/segments,c=std::cos(angle),s=std::sin(angle); glVertex3f(c*width,s*width,-length*0.5f); glVertex3f(c*width*0.32f,s*width*0.32f,length*0.5f);} glEnd(); glPopMatrix();
}

struct CpuCanvas {
    int w = PhoneDisplayState::LogicalWidth;
    int h = PhoneDisplayState::LogicalHeight;
    std::vector<unsigned char>& pixels;
};

void cpuClear(CpuCanvas& canvas, float r, float g, float b, float a = 1.0f) {
    const unsigned char rr = static_cast<unsigned char>(clampf(r, 0.0f, 1.0f) * 255.0f);
    const unsigned char gg = static_cast<unsigned char>(clampf(g, 0.0f, 1.0f) * 255.0f);
    const unsigned char bb = static_cast<unsigned char>(clampf(b, 0.0f, 1.0f) * 255.0f);
    const unsigned char aa = static_cast<unsigned char>(clampf(a, 0.0f, 1.0f) * 255.0f);
    for (int i = 0; i < canvas.w * canvas.h; ++i) {
        const int at = i * 4;
        canvas.pixels[at + 0] = rr;
        canvas.pixels[at + 1] = gg;
        canvas.pixels[at + 2] = bb;
        canvas.pixels[at + 3] = aa;
    }
}

void cpuRect(CpuCanvas& canvas, float x, float y, float w, float h, float r, float g, float b, float a) {
    const int x0 = std::max(0, static_cast<int>(std::floor(x)));
    const int y0 = std::max(0, static_cast<int>(std::floor(y)));
    const int x1 = std::min(canvas.w, static_cast<int>(std::ceil(x + w)));
    const int y1 = std::min(canvas.h, static_cast<int>(std::ceil(y + h)));
    const float alpha = clampf(a, 0.0f, 1.0f);
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const int at = (py * canvas.w + px) * 4;
            canvas.pixels[at + 0] = static_cast<unsigned char>((static_cast<float>(canvas.pixels[at + 0]) * (1.0f - alpha) + r * 255.0f * alpha));
            canvas.pixels[at + 1] = static_cast<unsigned char>((static_cast<float>(canvas.pixels[at + 1]) * (1.0f - alpha) + g * 255.0f * alpha));
            canvas.pixels[at + 2] = static_cast<unsigned char>((static_cast<float>(canvas.pixels[at + 2]) * (1.0f - alpha) + b * 255.0f * alpha));
            canvas.pixels[at + 3] = 255;
        }
    }
}

void cpuMeter(CpuCanvas& canvas, float x, float y, float w, float h, float fill, VisualColor color, float alpha = 0.82f) {
    cpuRect(canvas, x, y, w, h, 0.005f, 0.020f, 0.025f, 0.68f);
    cpuRect(canvas, x, y, w, 2.0f, VisualIdentity::ElectricCyan.r, VisualIdentity::ElectricCyan.g, VisualIdentity::ElectricCyan.b, 0.30f);
    cpuRect(canvas, x, y + h - 2.0f, w, 2.0f, VisualIdentity::ElectricCyan.r, VisualIdentity::ElectricCyan.g, VisualIdentity::ElectricCyan.b, 0.18f);
    cpuRect(canvas, x, y, w * clampf(fill, 0.0f, 1.0f), h, color.r, color.g, color.b, alpha);
}

const MenuFontAtlas& cpuMenuFont(bool semibold) {
    return semibold && menuSemiboldFont.cpuReady ? menuSemiboldFont : menuRegularFont;
}

float cpuTextWidth(const std::string& text, float px, bool semibold = false) {
    const MenuFontAtlas& font = cpuMenuFont(semibold);
    if (!font.cpuReady) return static_cast<float>(text.size()) * px * 0.55f;
    const float scale = stbtt_ScaleForPixelHeight(&font.info, px);
    float width = 0.0f;
    int previous = 0;
    for (unsigned char c : text) {
        int advance = 0, bearing = 0;
        stbtt_GetCodepointHMetrics(&font.info, c, &advance, &bearing);
        if (previous) width += static_cast<float>(stbtt_GetCodepointKernAdvance(&font.info, previous, c)) * scale;
        width += static_cast<float>(advance) * scale;
        previous = c;
    }
    return width;
}

void cpuText(CpuCanvas& canvas, const std::string& text, float x, float baseline, float px, float r, float g, float b, float a, bool semibold = false, bool centered = false) {
    const MenuFontAtlas& font = cpuMenuFont(semibold);
    if (!font.cpuReady) return;
    float pen = centered ? x - cpuTextWidth(text, px, semibold) * 0.5f : x;
    const float scale = stbtt_ScaleForPixelHeight(&font.info, px);
    int previous = 0;
    for (unsigned char c : text) {
        int advance = 0, bearing = 0;
        stbtt_GetCodepointHMetrics(&font.info, c, &advance, &bearing);
        if (previous) pen += static_cast<float>(stbtt_GetCodepointKernAdvance(&font.info, previous, c)) * scale;
        int bw = 0, bh = 0, xoff = 0, yoff = 0;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(&font.info, scale, scale, c, &bw, &bh, &xoff, &yoff);
        if (bw > 0 && bh > 0) {
            const int dstX = static_cast<int>(std::floor(pen)) + xoff;
            const int dstY = static_cast<int>(std::floor(baseline)) + yoff;
            for (int yy = 0; yy < bh; ++yy) {
                const int py = dstY + yy;
                if (py < 0 || py >= canvas.h) continue;
                for (int xx = 0; xx < bw; ++xx) {
                    const int pxOut = dstX + xx;
                    if (pxOut < 0 || pxOut >= canvas.w) continue;
                    const float alpha = (static_cast<float>(bitmap[yy * bw + xx]) / 255.0f) * clampf(a, 0.0f, 1.0f);
                    const int at = (py * canvas.w + pxOut) * 4;
                    canvas.pixels[at + 0] = static_cast<unsigned char>(static_cast<float>(canvas.pixels[at + 0]) * (1.0f - alpha) + r * 255.0f * alpha);
                    canvas.pixels[at + 1] = static_cast<unsigned char>(static_cast<float>(canvas.pixels[at + 1]) * (1.0f - alpha) + g * 255.0f * alpha);
                    canvas.pixels[at + 2] = static_cast<unsigned char>(static_cast<float>(canvas.pixels[at + 2]) * (1.0f - alpha) + b * 255.0f * alpha);
                    canvas.pixels[at + 3] = 255;
                }
            }
        }
        stbtt_FreeBitmap(bitmap, nullptr);
        pen += static_cast<float>(advance) * scale;
        previous = c;
    }
}

float cpuStencilTextWidth(const std::string& text, float px) {
    if (text.empty()) return 0.0f;
    return cpuTextWidth(text, px, true) * phone_stencil::HorizontalScale +
        static_cast<float>(text.size() - 1) * px * phone_stencil::TrackingEm;
}

void cpuStencilText(CpuCanvas& canvas,const std::string& text,float x,float baseline,float px,
                    float r,float g,float b,float a,float age,bool centered=false) {
    const MenuFontAtlas& font=cpuMenuFont(true);if(!font.cpuReady||text.empty())return;
    float pen=centered?x-cpuStencilTextWidth(text,px)*0.5f:x;
    const float scale=stbtt_ScaleForPixelHeight(&font.info,px);
    std::uint32_t seed=phone_stencil::textSeed(text);const bool leftToRight=(seed&1u)==0u;int previous=0;
    for(std::size_t i=0;i<text.size();++i){
        const unsigned char c=static_cast<unsigned char>(text[i]);seed=phone_stencil::glyphSeed(seed,c,i);
        if(previous)pen+=static_cast<float>(stbtt_GetCodepointKernAdvance(&font.info,previous,c))*scale*phone_stencil::HorizontalScale;
        int advance=0,bearing=0;stbtt_GetCodepointHMetrics(&font.info,c,&advance,&bearing);
        const float reveal=phone_stencil::glyphReveal(age,i,text.size(),leftToRight);
        const float travel=(leftToRight?-1.0f:1.0f)*(1.0f-reveal)*px*0.055f;
        const float jitterX=(static_cast<float>((seed>>4)&7u)-3.5f)*0.12f,jitterY=(static_cast<float>((seed>>9)&7u)-3.5f)*0.11f;
        if(c!=' '){
            int bw=0,bh=0,xoff=0,yoff=0;unsigned char* bitmap=stbtt_GetCodepointBitmap(&font.info,scale,scale,c,&bw,&bh,&xoff,&yoff);
            const int compressedW=std::max(1,static_cast<int>(std::ceil(static_cast<float>(bw)*phone_stencil::HorizontalScale)));
            const bool bridge=compressedW>=8&&c!='I'&&c!='i'&&c!='l'&&c!='1';const int bridgeX=static_cast<int>(std::round((0.30f+static_cast<float>((seed>>13)&3u)*0.13f)*static_cast<float>(compressedW-1)));
            if(bitmap&&bw>0&&bh>0)for(int yy=0;yy<bh;++yy)for(int dx=0;dx<compressedW;++dx){
                const float yn=bh>1?static_cast<float>(yy)/static_cast<float>(bh-1):0.0f;if(bridge&&reveal<0.96f&&std::abs(dx-bridgeX)<=(px>=42.0f?1:0)&&yn>0.20f&&yn<0.80f)continue;
                const int sx=std::min(bw-1,static_cast<int>(static_cast<float>(dx)/phone_stencil::HorizontalScale));unsigned char ink=0;
                for(int sy=std::max(0,yy-1);sy<=std::min(bh-1,yy+1);++sy)for(int nx=std::max(0,sx-1);nx<=std::min(bw-1,sx+1);++nx)ink=std::max(ink,bitmap[sy*bw+nx]);
                const float alpha=static_cast<float>(ink)/255.0f*a*reveal;if(alpha<=0.01f)continue;
                const int ox=static_cast<int>(std::floor(pen+static_cast<float>(xoff)*phone_stencil::HorizontalScale+static_cast<float>(dx)+jitterX+travel)),oy=static_cast<int>(std::floor(baseline+static_cast<float>(yoff+yy)+jitterY));if(ox<0||ox>=canvas.w||oy<0||oy>=canvas.h)continue;
                const int at=(oy*canvas.w+ox)*4;canvas.pixels[at]=static_cast<unsigned char>(canvas.pixels[at]*(1-alpha)+r*255*alpha);canvas.pixels[at+1]=static_cast<unsigned char>(canvas.pixels[at+1]*(1-alpha)+g*255*alpha);canvas.pixels[at+2]=static_cast<unsigned char>(canvas.pixels[at+2]*(1-alpha)+b*255*alpha);canvas.pixels[at+3]=255;
            }
            stbtt_FreeBitmap(bitmap,nullptr);
            if((seed&3u)!=0u&&reveal>0.0f&&reveal<1.0f){const float fleck=std::max(1.0f,px*0.025f);cpuRect(canvas,pen+jitterX+travel*1.7f,baseline-px*0.45f,fleck,fleck,r,g,b,a*0.28f);}
        }
        pen+=static_cast<float>(advance)*scale*phone_stencil::HorizontalScale+px*phone_stencil::TrackingEm;previous=c;
    }
}

// Selection paint belongs to each glyph, not to a stretched panel texture.
// The full label seeds stable variation so it feels handmade without flicker.
void cpuLetterSplatter(CpuCanvas& canvas,const std::string& text,float x,float baseline,float px,
                       const VisualColor& color,float opacity,int gesture,bool centered=false) {
    const MenuFontAtlas& font=cpuMenuFont(true);if(!font.cpuReady||text.empty()||opacity<=0.0f)return;
    float pen=centered?x-cpuStencilTextWidth(text,px)*0.5f:x;
    const float scale=stbtt_ScaleForPixelHeight(&font.info,px);
    std::uint32_t seed=phone_stencil::textSeed(text)^static_cast<std::uint32_t>(gesture*0x9e3779b9u);int previous=0;
    const float direction=gesture==2?-1.0f:(gesture==3?1.0f:0.0f);
    for(std::size_t i=0;i<text.size();++i){
        const unsigned char c=static_cast<unsigned char>(text[i]);seed=phone_stencil::glyphSeed(seed,c,i);
        if(previous)pen+=static_cast<float>(stbtt_GetCodepointKernAdvance(&font.info,previous,c))*scale*phone_stencil::HorizontalScale;
        int advance=0,bearing=0;stbtt_GetCodepointHMetrics(&font.info,c,&advance,&bearing);
        if(c!=' '){
            const std::string glyph(1,static_cast<char>(c));
            const float randomX=(static_cast<float>((seed>>4)&15u)-7.5f)*px*0.018f;
            const float randomY=(static_cast<float>((seed>>9)&15u)-7.5f)*px*0.012f;
            const float thrust=direction*(px*(0.10f+static_cast<float>((seed>>14)&7u)*0.015f));
            cpuStencilText(canvas,glyph,pen+randomX+thrust,baseline+randomY,px,color.r,color.g,color.b,opacity*0.30f,1.0f);
            cpuStencilText(canvas,glyph,pen-randomX*0.42f+thrust*0.48f,baseline-randomY*0.55f,px,color.r,color.g,color.b,opacity*0.18f,1.0f);
            const float drop=std::max(1.0f,px*(0.025f+static_cast<float>((seed>>18)&3u)*0.008f));
            cpuRect(canvas,pen+randomX+thrust*1.35f,baseline-px*(0.30f+static_cast<float>((seed>>21)&7u)*0.045f),drop,drop,color.r,color.g,color.b,opacity*0.55f);
        }
        pen+=static_cast<float>(advance)*scale*phone_stencil::HorizontalScale+px*phone_stencil::TrackingEm;previous=c;
    }
}

void drawPaletteMenuTitle(const std::string& text, float centerX, float centerY, float px, float time, float opacity = 1.0f) {
    const MenuFontAtlas& font = cpuMenuFont(true);
    if (!font.cpuReady || text.empty()) return;
    constexpr float rasterSupersample = 2.0f;
    constexpr float rasterToScreen = 1.0f / rasterSupersample;
    struct CachedTitleGlyph {
        unsigned char code = 0;
        int bw = 0, bh = 0, xoff = 0, yoff = 0, advance = 0;
        std::vector<unsigned char> bitmap;
    };
    static float cachedPx = -1.0f;
    static std::vector<CachedTitleGlyph> cachedGlyphs;
    const float baseline = centerY + px * 0.34f;
    const float scale = stbtt_ScaleForPixelHeight(&font.info, px);
    const float rasterScale = stbtt_ScaleForPixelHeight(&font.info, px * rasterSupersample);
    if (std::abs(cachedPx - px) > 0.01f || cachedGlyphs.size() != text.size()) {
        cachedPx = px;
        cachedGlyphs.clear();
        cachedGlyphs.reserve(text.size());
        for (unsigned char c : text) {
            CachedTitleGlyph glyph;
            glyph.code = c;
            int bearing = 0;
            stbtt_GetCodepointHMetrics(&font.info, c, &glyph.advance, &bearing);
            unsigned char* bitmap = stbtt_GetCodepointBitmap(&font.info, rasterScale, rasterScale, c, &glyph.bw, &glyph.bh, &glyph.xoff, &glyph.yoff);
            if (bitmap && glyph.bw > 0 && glyph.bh > 0)
                glyph.bitmap.assign(bitmap, bitmap + glyph.bw * glyph.bh);
            stbtt_FreeBitmap(bitmap, nullptr);
            cachedGlyphs.push_back(std::move(glyph));
        }
    }
    float pen = centerX - cpuTextWidth(text, px, true) * 0.5f;
    int previous = 0;
    glBegin(GL_QUADS);
    for (std::size_t i = 0; i < text.size(); ++i) {
        const CachedTitleGlyph& glyph = cachedGlyphs[i];
        const unsigned char c = glyph.code;
        if (previous) pen += static_cast<float>(stbtt_GetCodepointKernAdvance(&font.info, previous, c)) * scale;
        const float signal=0.08f+0.06f*std::sin(time*1.8f+static_cast<float>(i)*1.7f);
        const VisualColor accent=(i+1==text.size())?VisualIdentity::ElectricMagenta:VisualIdentity::ElectricCyan;
        const float r=0.82f+(accent.r-0.82f)*signal;
        const float g=0.94f+(accent.g-0.94f)*signal;
        const float b=0.98f+(accent.b-0.98f)*signal;
        for (int yy = 0; yy < glyph.bh; ++yy) for (int xx = 0; xx < glyph.bw; ++xx) {
            const float alpha = static_cast<float>(glyph.bitmap[yy * glyph.bw + xx]) / 255.0f * 0.96f * opacity;
            if (alpha <= 0.01f) continue;
            const float x = pen + static_cast<float>(glyph.xoff + xx) * rasterToScreen;
            const float y = baseline + static_cast<float>(glyph.yoff + yy) * rasterToScreen;
            glColor4f(r, g, b, alpha);
            glVertex2f(x, y); glVertex2f(x + rasterToScreen, y); glVertex2f(x + rasterToScreen, y + rasterToScreen); glVertex2f(x, y + rasterToScreen);
        }
        pen += static_cast<float>(glyph.advance) * scale;
        previous = c;
    }
    glEnd();
}

void renderPhoneDisplayPixels(const GameState& state, std::vector<unsigned char>& pixels) {
    pixels.resize(PhoneDisplayState::LogicalWidth * PhoneDisplayState::LogicalHeight * 4);
    CpuCanvas canvas{PhoneDisplayState::LogicalWidth, PhoneDisplayState::LogicalHeight, pixels};
    const PhoneDisplayState& display = state.phoneDisplay;
    if (state.dead || display.mode == PhoneDisplayMode::Off || display.mode == PhoneDisplayMode::Death) {
        cpuClear(canvas, 0.006f, 0.010f, 0.013f, 1.0f);
        return;
    }
    const Vec3 tint = display.screenTint;
    cpuClear(canvas, 0.025f + tint.x * 0.18f, 0.045f + tint.y * 0.16f, 0.060f + tint.z * 0.15f, 1.0f);
    cpuRect(canvas, 0, 0, static_cast<float>(canvas.w), static_cast<float>(canvas.h), 0.03f, 0.55f, 0.62f, 0.10f + display.brightness * 0.08f);

    const bool menuVisible = state.cinematic.introActive || !state.started ||
        (state.started && state.uiPaused && !state.multiplayer.enabled && !state.upgradeMenu.active);
    if (!menuVisible) {
        const GameplayPhoneModel phone = makeGameplayPhoneModel(state);
        const float vacuum = clampf(state.vacuum.power, 0.0f, 1.0f);
        const float activity = clampf(vacuum * 0.70f + display.powerPulse * 0.20f + display.capturePulse * 0.20f, 0.0f, 1.0f);
        const VisualColor stable = VisualIdentity::ElectricCyan;
        const VisualColor stored = VisualIdentity::AcidChartreuse;
        const VisualColor warning = VisualIdentity::Copper;
        const VisualColor batteryColor = phone.lowBattery ? warning : stable;
        const float reveal = phone_stencil::appearanceAge(display.transitionProgress);

        cpuStencilText(canvas,"ROOM "+std::to_string(phone.roomIndex),84.0f,112.0f,30.0f,
            stable.r,stable.g,stable.b,0.62f,reveal);
        const std::string tokenText="TOKEN "+std::to_string(phone.tokens);
        const float tokenPx=30.0f;
        cpuStencilText(canvas,tokenText,636.0f-cpuTextWidth(tokenText,tokenPx,false),112.0f,tokenPx,
            stored.r,stored.g,stored.b,0.62f,reveal);

        const int objectiveCount=std::max(1,phone.requiredGoals);
        const float objectiveGap=12.0f;
        const float objectiveCell=std::min(46.0f,(552.0f-objectiveGap*static_cast<float>(objectiveCount-1))/static_cast<float>(objectiveCount));
        const float objectiveW=objectiveCell*objectiveCount+objectiveGap*(objectiveCount-1);
        const float objectiveX=(720.0f-objectiveW)*0.5f;
        for(int i=0;i<objectiveCount;++i){
            const bool filled=i<phone.filledGoals;
            const VisualColor color=filled?stored:stable;
            const float x=objectiveX+i*(objectiveCell+objectiveGap);
            cpuRect(canvas,x,164.0f,objectiveCell,objectiveCell,color.r,color.g,color.b,filled?0.92f:0.38f);
            cpuRect(canvas,x+4.0f,168.0f,objectiveCell-8.0f,objectiveCell-8.0f,0.008f,0.020f,0.026f,filled?0.18f:0.86f);
        }

        cpuStencilText(canvas,"CHARGE",84.0f,300.0f,34.0f,
            batteryColor.r,batteryColor.g,batteryColor.b,0.78f,reveal);
        const std::string batteryValue=std::to_string(phone.batteryPercent);
        cpuStencilText(canvas,batteryValue,636.0f-cpuTextWidth(batteryValue,54.0f,true),310.0f,54.0f,
            batteryColor.r,batteryColor.g,batteryColor.b,0.94f,reveal);
        cpuRect(canvas,84.0f,338.0f,552.0f,18.0f,0.008f,0.020f,0.026f,0.92f);
        cpuRect(canvas,84.0f,338.0f,552.0f*phone.batteryFill,18.0f,
            batteryColor.r,batteryColor.g,batteryColor.b,0.90f);

        constexpr int columns=5,rows=6,cells=columns*rows;
        const float cell=54.0f,gap=12.0f,gridW=columns*cell+(columns-1)*gap;
        const float gridX=(720.0f-gridW)*0.5f,gridY=470.0f;
        for(int i=0;i<cells;++i){
            const int col=i%columns,row=i/columns;
            const VisualColor c=VisualIdentity::DataMosaicPalette[i%25];
            const bool filled=i<phone.storedSouls;
            const float wave=0.5f+0.5f*std::sin(state.time*0.9f+i*0.37f+display.screenNoisePhase*8.0f);
            const float alpha=filled?(0.76f+activity*0.16f+wave*0.08f):0.10f;
            const float shade=filled?1.0f:0.24f;
            cpuRect(canvas,gridX+col*(cell+gap),gridY+row*(cell+gap),cell,cell,
                c.r*shade,c.g*shade,c.b*shade,alpha);
        }
        const std::string soulValue=std::to_string(phone.storedSouls)+" / "+std::to_string(phone.soulCapacity);
        cpuStencilText(canvas,soulValue,360.0f-cpuTextWidth(soulValue,42.0f,true)*0.5f,930.0f,42.0f,
            stored.r,stored.g,stored.b,0.88f,reveal);

        const std::string memoryValue="MEMORY "+std::to_string(phone.unlockedMaterialMemories)+" / 3";
        cpuStencilText(canvas,memoryValue,360.0f-cpuTextWidth(memoryValue,25.0f,false)*0.5f,1136.0f,25.0f,
            stable.r,stable.g,stable.b,0.48f,reveal);

        if(phone.supplementalActive){
            const std::string power="POWER "+std::to_string(phone.flowerStacks);
            cpuStencilText(canvas,power,84.0f,1030.0f,30.0f,stored.r,stored.g,stored.b,0.74f,reveal);
            cpuRect(canvas,84.0f,1052.0f,552.0f,10.0f,0.008f,0.020f,0.026f,0.90f);
            cpuRect(canvas,84.0f,1052.0f,552.0f*phone.supplementalFill,10.0f,stored.r,stored.g,stored.b,0.82f);
        }
        if (phone.lowBattery) {
            const float pulse = 0.35f + 0.25f * std::sin(state.time * 4.1f);
            cpuRect(canvas,0,0,static_cast<float>(canvas.w),18.0f,warning.r,warning.g,warning.b,pulse);
            cpuRect(canvas,0,static_cast<float>(canvas.h)-18.0f,static_cast<float>(canvas.w),18.0f,warning.r,warning.g,warning.b,pulse);
        }
        return;
    }

    const PhoneDisplayMenuLayout layout = makePhoneDisplayMenuLayout(state, cpuTextWidth);
    const float stencilAge=phone_stencil::appearanceAge(display.transitionProgress);
    const Vec3 resolvedAccent=phoneDisplayResolvedAccent(display);
    const VisualColor channelAccent{resolvedAccent.x,resolvedAccent.y,resolvedAccent.z};
    if (!layout.title.empty()) {
        if (layout.paletteTitle) {
            float pen = layout.logicalW * 0.5f - cpuTextWidth(layout.title, layout.titlePx, true) * 0.5f;
            for (std::size_t i = 0; i < layout.title.size(); ++i) {
                const std::string letter(1, layout.title[i]);
                const float hue = std::fmod(state.time * 0.026f + static_cast<float>(i) * 0.115f, 1.0f);
                const float k = hue * 6.0f, f = k - std::floor(k), q = 1.0f - f;
                Vec3 color{1.0f, f, 0.0f};
                switch (static_cast<int>(k) % 6) {
                    case 1: color = {q, 1.0f, 0.0f}; break;
                    case 2: color = {0.0f, 1.0f, f}; break;
                    case 3: color = {0.0f, q, 1.0f}; break;
                    case 4: color = {f, 0.0f, 1.0f}; break;
                    case 5: color = {1.0f, 0.0f, q}; break;
                }
                cpuText(canvas, letter, pen, layout.titleCenterY + layout.titlePx * 0.34f, layout.titlePx, 0.55f + color.x * 0.42f, 0.65f + color.y * 0.34f, 0.72f + color.z * 0.28f, 0.96f, true);
                pen += cpuTextWidth(letter, layout.titlePx, true) + 2.0f;
            }
        } else {
            cpuStencilText(canvas,layout.title,layout.logicalW*0.5f,layout.titleCenterY+layout.titlePx*0.34f,layout.titlePx,0.90f,0.97f,1.0f,0.96f,stencilAge,true);
        }
    }
    if (layout.joinCode) {
        const std::string room = state.multiplayer.roomCode.data();
        std::string typed;
        for (int i = 0; i < 6; ++i) { typed += i < static_cast<int>(room.size()) ? room[i] : '_'; if (i < 5) typed += ' '; }
        cpuStencilText(canvas,typed,layout.logicalW*0.5f,layout.content.y+layout.content.h*0.50f,46.0f,0.88f,1.0f,1.0f,0.94f,stencilAge,true);
    }
    for (int i = 0; i < layout.rowCount; ++i) {
        const PhoneDisplayMenuRow& row = layout.rows[i];
        if (!row.visible) continue;
        const bool selected = row.selectable && state.hud.menuSelection == row.selectableIndex;
        if (row.peek) {
            const float top = std::max(row.visual.y, layout.content.y);
            const float bottom = std::min(row.visual.y + row.visual.h, layout.content.y + layout.content.h);
            const float height = std::max(0.0f, bottom - top);
            cpuRect(canvas, row.visual.x + 26.0f, top, row.visual.w - 52.0f, height,
                    VisualIdentity::MetallicTeal.r, VisualIdentity::MetallicTeal.g,
                    VisualIdentity::MetallicTeal.b, 0.24f);
            continue;
        }
        if (row.kind == PhoneMenuRowKind::Section) {
            cpuStencilText(canvas,row.label,row.labelX,row.baselineY,row.fontPx,VisualIdentity::MetallicTeal.r,VisualIdentity::MetallicTeal.g,VisualIdentity::MetallicTeal.b,0.62f,stencilAge);
            continue;
        }
        const PhoneMenuEmphasis emphasis=phoneMenuEmphasis(row.action);
        const float response=selected?clampf(state.cinematic.textInteraction,0.0f,1.0f):0.0f;
        const int gesture=response>0.035f?state.cinematic.menuPaintGesture:0;
        const VisualColor focusColor=emphasis==PhoneMenuEmphasis::Destructive?VisualIdentity::Copper:
            (emphasis==PhoneMenuEmphasis::Primary?VisualIdentity::AcidChartreuse:channelAccent);
        const float alpha = selected ? 1.0f : 0.72f;
        if (row.kind == PhoneMenuRowKind::TwoColumn) {
            const VisualColor labelColor=emphasis==PhoneMenuEmphasis::Destructive?VisualIdentity::Copper:VisualColor{selected?1.0f:0.70f,selected?1.0f:0.88f,1.0f};
            if(selected)cpuLetterSplatter(canvas,row.label,row.labelX,row.baselineY,row.fontPx,focusColor,0.36f+response*0.34f,gesture);
            cpuStencilText(canvas,row.label,row.labelX,row.baselineY,row.fontPx,labelColor.r,labelColor.g,labelColor.b,alpha,stencilAge);
            const float valueWidth=cpuStencilTextWidth(row.value,row.fontPx);
            const float valueLeft=row.valueRightX-valueWidth;
            if(selected)cpuLetterSplatter(canvas,row.value,valueLeft,row.baselineY,row.fontPx,focusColor,0.28f+response*0.26f,gesture);
            cpuStencilText(canvas,row.value,valueLeft,row.baselineY,row.fontPx,selected?channelAccent.r:VisualIdentity::MetallicTeal.r,selected?channelAccent.g:VisualIdentity::MetallicTeal.g,selected?channelAccent.b:VisualIdentity::MetallicTeal.b,selected?0.98f:0.78f,stencilAge);
            const float amount=phoneMenuVisualAmount(row.action,state.localSettings);
            if(amount>=0.0f&&row.horizontal==PhoneMenuHorizontal::Adjust){
                const float trackW=150.0f,trackH=5.0f;
                const float trackX=row.valueRightX-trackW,trackY=row.baselineY+13.0f;
                cpuRect(canvas,trackX,trackY,trackW,trackH,VisualIdentity::DeepPlum.r,VisualIdentity::DeepPlum.g,VisualIdentity::DeepPlum.b,0.58f);
                const VisualColor fill=selected?channelAccent:VisualIdentity::MetallicTeal;
                cpuRect(canvas,trackX,trackY,std::max(trackH,trackW*amount),trackH,fill.r,fill.g,fill.b,selected?0.94f:0.62f);
                const float thumbX=trackX+clampf(amount,0.0f,1.0f)*(trackW-trackH);
                cpuRect(canvas,thumbX,trackY-2.0f,trackH,trackH+4.0f,fill.r,fill.g,fill.b,selected?1.0f:0.76f);
            }
        } else {
            const bool centered=state.dead&&row.action==PhoneMenuAction::Restart;
            const VisualColor actionColor=emphasis==PhoneMenuEmphasis::Destructive?VisualIdentity::Copper:
                (selected&&emphasis==PhoneMenuEmphasis::Primary?VisualIdentity::AcidChartreuse:VisualColor{selected?1.0f:0.70f,selected?1.0f:0.88f,1.0f});
            if(selected)cpuLetterSplatter(canvas,row.label,centered?layout.logicalW*0.5f:row.labelX,row.baselineY,row.fontPx,focusColor,0.38f+response*0.36f,gesture,centered);
            cpuStencilText(canvas,row.label,centered?layout.logicalW*0.5f:row.labelX,row.baselineY,row.fontPx,actionColor.r,actionColor.g,actionColor.b,alpha,stencilAge,centered);
        }
    }
    if (!layout.navigationHint.empty()) {
        const float hintPx=fitPhoneDisplayTextPx(layout.navigationHint,26.0f,layout.safe.w,false,cpuTextWidth);
        cpuStencilText(canvas,layout.navigationHint,layout.logicalW*0.5f,layout.logicalH-66.0f,
            hintPx,channelAccent.r,channelAccent.g,channelAccent.b,0.62f,stencilAge,true);
    }
}

std::uint64_t phoneDisplayRenderKey(const GameState& state) {
    std::uint64_t hash = 1469598103934665603ull;
    const PhoneDisplayState& display = state.phoneDisplay;
    hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(display.mode));
    hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(display.previousMode));
    hashPhoneDisplayValue(hash, display.interactive ? 1u : 0u);
    hashPhoneDisplayFloat(hash, display.brightness, 255.0f);
    hashPhoneDisplayFloat(hash, display.contentOpacity, 255.0f);
    hashPhoneDisplayFloat(hash, display.transitionProgress, 240.0f);
    hashPhoneDisplayFloat(hash, display.screenTint.x, 255.0f);
    hashPhoneDisplayFloat(hash, display.screenTint.y, 255.0f);
    hashPhoneDisplayFloat(hash, display.screenTint.z, 255.0f);
    hashPhoneDisplayFloat(hash, display.material.rimEmission, 255.0f);
    hashPhoneDisplayValue(hash, state.started ? 1u : 0u);
    hashPhoneDisplayValue(hash, state.dead ? 1u : 0u);
    hashPhoneDisplayValue(hash, state.uiPaused ? 1u : 0u);
    hashPhoneDisplayValue(hash, state.cinematic.introActive ? 1u : 0u);
    hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(state.localSettings.menuPage));
    hashPhoneDisplayFloat(hash, state.localSettings.menuScroll, 10.0f);
    hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(std::max(0, state.hud.menuSelection)));
    hashPhoneDisplayFloat(hash, state.cinematic.textInteraction, 120.0f);
    hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(std::max(0, std::min(3, state.cinematic.menuPaintGesture))));
    hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(std::max(-1, state.localSettings.rebindingAction) + 1));
    hashPhoneDisplayValue(hash, state.localSettings.musicMuted ? 1u : 0u);
    hashPhoneDisplayValue(hash, state.localSettings.sfxMuted ? 1u : 0u);
    hashPhoneDisplayValue(hash, state.localSettings.shadows ? 1u : 0u);
    hashPhoneDisplayValue(hash, state.localSettings.particles ? 1u : 0u);
    hashPhoneDisplayValue(hash, state.localSettings.fpsCounter ? 1u : 0u);
    hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(std::max(0, state.localSettings.graphicsPreset)));
    hashPhoneDisplayFloat(hash, state.localSettings.musicVolume, 100.0f);
    hashPhoneDisplayFloat(hash, state.localSettings.sfxVolume, 100.0f);
    hashPhoneDisplayFloat(hash, state.localSettings.mouseLookSensitivity, 100.0f);
    hashPhoneDisplayFloat(hash, state.localSettings.controllerLookSensitivity, 100.0f);
    hashPhoneDisplayValue(hash, state.localSettings.controllerTriggerSensitivity);
    hashPhoneDisplayValue(hash, state.localSettings.controllerVibration);
    hashPhoneDisplayFloat(hash, state.vacuum.power, 240.0f);
    hashPhoneDisplayFloat(hash, state.hud.criticalHitPulse, 120.0f);
    hashPhoneDisplayValue(hash, state.hud.lowBattery ? 1u : 0u);
    const GameplayPhoneModel phone=makeGameplayPhoneModel(state);
    hashPhoneDisplayFloat(hash,phone.batteryFill,100.0f);
    hashPhoneDisplayValue(hash,static_cast<std::uint64_t>(phone.storedSouls));
    hashPhoneDisplayValue(hash,static_cast<std::uint64_t>(phone.filledGoals));
    hashPhoneDisplayValue(hash,static_cast<std::uint64_t>(phone.requiredGoals));
    hashPhoneDisplayValue(hash,phone.roomClear?1u:0u);
    hashPhoneDisplayValue(hash,static_cast<std::uint64_t>(phone.roomIndex));
    hashPhoneDisplayValue(hash,static_cast<std::uint64_t>(phone.tokens));
    hashPhoneDisplayValue(hash,phone.supplementalActive?1u:0u);
    hashPhoneDisplayFloat(hash,phone.supplementalFill,100.0f);
    hashPhoneDisplayValue(hash,static_cast<std::uint64_t>(phone.flowerStacks));
    for (int key : state.localSettings.keyboardBindings) {
        hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(std::max(0, key)));
    }
    hashPhoneDisplayString(hash, state.multiplayer.roomCode.data());

    const PhoneMenuPageViewModel page = makePhoneMenuPageModel(state);
    hashPhoneDisplayString(hash, page.title);
    hashPhoneDisplayValue(hash, page.paletteTitle ? 1u : 0u);
    hashPhoneDisplayValue(hash, page.joinCode ? 1u : 0u);
    hashPhoneDisplayValue(hash, page.tablePage ? 1u : 0u);
    hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(page.elementCount));
    for (int i = 0; i < page.elementCount; ++i) {
        const PhoneMenuElement& element = page.elements[i];
        hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(element.kind));
        hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(element.action));
        hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(element.horizontal));
        hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(std::max(-1, element.bindingAction) + 1));
        hashPhoneDisplayValue(hash, element.selectable ? 1u : 0u);
        hashPhoneDisplayString(hash, element.label);
        hashPhoneDisplayString(hash, element.value);
    }
    if (page.paletteTitle) {
        hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(std::floor(state.time * 15.0f)));
    }
    const PhoneMenuElement* selectedElement=phoneMenuElementForSelection(page,state.hud.menuSelection);
    if(selectedElement&&selectedElement->horizontal==PhoneMenuHorizontal::Adjust)
        hashPhoneDisplayValue(hash,static_cast<std::uint64_t>(std::floor(state.time*8.0f)));
    if (!state.dead && state.started && !state.uiPaused) {
        hashPhoneDisplayValue(hash, static_cast<std::uint64_t>(std::floor(state.time * 10.0f)));
        hashPhoneDisplayFloat(hash, display.powerPulse, 100.0f);
        hashPhoneDisplayFloat(hash, display.capturePulse, 100.0f);
        hashPhoneDisplayFloat(hash, display.screenNoisePhase, 20.0f);
    }
    return hash;
}

void DesktopRenderer::drawBox(const Vec3& p, const Vec3& s, const Quat& q, float r, float g, float b, float a) {
    const float matrix[16] = {
        1-2*(q.y*q.y+q.z*q.z), 2*(q.x*q.y+q.z*q.w), 2*(q.x*q.z-q.y*q.w), 0,
        2*(q.x*q.y-q.z*q.w), 1-2*(q.x*q.x+q.z*q.z), 2*(q.y*q.z+q.x*q.w), 0,
        2*(q.x*q.z+q.y*q.w), 2*(q.y*q.z-q.x*q.w), 1-2*(q.x*q.x+q.y*q.y), 0,
        0,0,0,1
    };
    glPushMatrix(); glTranslatef(p.x,p.y,p.z); glMultMatrixf(matrix); glScalef(s.x,s.y,s.z);
    gradedColor(r,g,b,a); cube(); glPopMatrix();
}

void DesktopRenderer::drawSecretTvScreen(const GameState& state, float phoneProximity) const {
    if(!tvGifWall_.available())return;
    if(!tvScreenTexture_)glGenTextures(1,&tvScreenTexture_);
    const float fullness=clampf(static_cast<float>(state.secretTv.signal)/24.0f,0.0f,1.0f);
    const float clarity=0.80f+0.20f*fullness;
    const float proximityWash=phoneProximity*(1.0f-fullness);
    const float flicker=1.0f-proximityWash*(0.035f+0.030f*std::sin(state.time*15.0f));
    const float brokenDim=state.secretTv.broken?0.38f:1.0f;
    unsigned char pixels[TvGifWall::Columns*TvGifWall::Rows*3]{};
    for(int y=0;y<TvGifWall::Rows;++y)for(int x=0;x<TvGifWall::Columns;++x){
        const auto color=tvGifWall_.sample(x,y,state.time,state.secretTv.signal);
        const float slowBand=1.0f-proximityWash*0.045f*std::sin(state.time*2.3f+static_cast<float>(y)*0.75f);
        const float gain=clampf(clarity*flicker*slowBand*brokenDim,0.0f,1.22f);
        const std::size_t at=static_cast<std::size_t>((TvGifWall::Rows-1-y)*TvGifWall::Columns+x)*3u;
        pixels[at+0]=static_cast<unsigned char>(clampf(color.r*gain,0.0f,1.0f)*255.0f);
        pixels[at+1]=static_cast<unsigned char>(clampf(color.g*gain,0.0f,1.0f)*255.0f);
        pixels[at+2]=static_cast<unsigned char>(clampf(color.b*gain,0.0f,1.0f)*255.0f);
    }
    glBindTexture(GL_TEXTURE_2D,tvScreenTexture_);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,TvGifWall::Columns,TvGifWall::Rows,0,GL_RGB,GL_UNSIGNED_BYTE,pixels);
    glDisable(GL_LIGHTING);glEnable(GL_TEXTURE_2D);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
    glColor4f(1.0f,1.0f,1.0f,state.secretTv.broken?0.74f:0.98f);
    constexpr float x=41.785f,cy=0.80f,cz=0.0f,halfY=0.455f,halfZ=0.655f;
    glBegin(GL_QUADS);
    glNormal3f(-1,0,0);
    glTexCoord2f(0,0);glVertex3f(x,cy-halfY,cz-halfZ);
    glTexCoord2f(1,0);glVertex3f(x,cy-halfY,cz+halfZ);
    glTexCoord2f(1,1);glVertex3f(x,cy+halfY,cz+halfZ);
    glTexCoord2f(0,1);glVertex3f(x,cy+halfY,cz-halfZ);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    const float sheen=0.10f+0.08f*std::sin(state.time*0.7f);
    glColor4f(VisualIdentity::ElectricCyan.r,VisualIdentity::ElectricCyan.g,VisualIdentity::ElectricCyan.b,0.08f+sheen*0.22f);
    glBegin(GL_QUADS);
    glVertex3f(x-0.002f,cy+halfY*0.82f,cz-halfZ);
    glVertex3f(x-0.002f,cy+halfY*0.82f,cz+halfZ);
    glVertex3f(x-0.002f,cy+halfY,cz+halfZ);
    glVertex3f(x-0.002f,cy+halfY,cz-halfZ);
    glEnd();
    glDepthMask(GL_TRUE);glDisable(GL_BLEND);glDisable(GL_TEXTURE_2D);glEnable(GL_LIGHTING);
}

void DesktopRenderer::drawPhoneDisplayTexture(const GameState& state) const {
    if (!phoneDisplayTexture_) glGenTextures(1, &phoneDisplayTexture_);
    glBindTexture(GL_TEXTURE_2D, phoneDisplayTexture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    const std::uint64_t renderKey = phoneDisplayRenderKey(state);
    if (!phoneDisplayCacheValid_ || renderKey != phoneDisplayCacheKey_ || !phoneDisplayTextureAllocated_) {
        renderPhoneDisplayPixels(state, phoneDisplayPixels_);
        if (!phoneDisplayTextureAllocated_) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PhoneDisplayState::LogicalWidth, PhoneDisplayState::LogicalHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, phoneDisplayPixels_.data());
            phoneDisplayTextureAllocated_ = true;
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, PhoneDisplayState::LogicalWidth, PhoneDisplayState::LogicalHeight, GL_RGBA, GL_UNSIGNED_BYTE, phoneDisplayPixels_.data());
        }
        phoneDisplayCacheKey_ = renderKey;
        phoneDisplayCacheValid_ = true;
    }

    const PhoneTransformState& phone = state.phoneTransform;
    const float halfW = PHONE_SCREEN_WIDTH * PHONE_DISPLAY_INSET_X * state.phoneVisual.screenScale.x * 0.5f;
    const float halfH = PHONE_SCREEN_HEIGHT * PHONE_DISPLAY_INSET_Y * state.phoneVisual.screenScale.y * 0.5f;
    const Vec3 center = phone.screenCenter + phone.screenNormal * PHONE_DISPLAY_PLANE_OFFSET;
    const Vec3 rx = phone.screenRight * halfW;
    const Vec3 uy = phone.screenUp * halfH;
    const PhoneDisplayState& display = state.phoneDisplay;
    const float alpha = clampf(0.12f + display.brightness * 0.88f, 0.0f, 1.0f);

    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glColor4f(1.0f, 1.0f, 1.0f, alpha);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex3f((center-rx-uy).x, (center-rx-uy).y, (center-rx-uy).z);
    glTexCoord2f(1, 1); glVertex3f((center+rx-uy).x, (center+rx-uy).y, (center+rx-uy).z);
    glTexCoord2f(1, 0); glVertex3f((center+rx+uy).x, (center+rx+uy).y, (center+rx+uy).z);
    glTexCoord2f(0, 0); glVertex3f((center-rx+uy).x, (center-rx+uy).y, (center-rx+uy).z);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}

void DesktopRenderer::drawStaticModel(unsigned int list, const Vec3& p, const Vec3& s, const Quat& q) {
    const float matrix[16] = {
        1-2*(q.y*q.y+q.z*q.z), 2*(q.x*q.y+q.z*q.w), 2*(q.x*q.z-q.y*q.w), 0,
        2*(q.x*q.y-q.z*q.w), 1-2*(q.x*q.x+q.z*q.z), 2*(q.y*q.z+q.x*q.w), 0,
        2*(q.x*q.z+q.y*q.w), 2*(q.y*q.z-q.x*q.w), 1-2*(q.x*q.x+q.y*q.y), 0,
        0,0,0,1
    };
    glPushMatrix(); glTranslatef(p.x,p.y,p.z); glMultMatrixf(matrix); glScalef(s.x,s.y,s.z); glCallList(list); glPopMatrix();
}

void DesktopRenderer::drawHumanModel(const TargetState& target,float time,room_environment::RoomSetting setting,bool shadow) const {
    const ScopedMaterial material(semanticMaterial(SemanticSurface::Organic),!shadow);
    const HumanBodyPresentation body=target.humanBodyPresentation();
    const Vec3 leftFoot=body.leftFootTarget(),rightFoot=body.rightFootTarget();
    HumanModelFootTargets footTargets{};
    footTargets.left[0]=leftFoot.x;footTargets.left[1]=leftFoot.y;footTargets.left[2]=leftFoot.z;
    footTargets.right[0]=rightFoot.x;footTargets.right[1]=rightFoot.y;footTargets.right[2]=rightFoot.z;
    footTargets.leftContact=body.leftFootContact();footTargets.rightContact=body.rightFootContact();
    footTargets.active=body.authoritative();
    const auto& reaction=target.visualReaction;
    humanModel_.skin(target.humanAnimationTime,target.attackTimer,target.attackVariant,
        humanVertices_,{target.hitFlash,target.hitDirectionLocal,time,reaction.locomotionPhase,
            reaction.locomotionAmount,reaction.commitment,reaction.searchAmount,reaction.disruption,
            reaction.vacuumPullAmount,reaction.individuality},footTargets);if(humanVertices_.empty())return;
    const bool aliveHuman=!target.slurpable;const HumanVisualPose pose=makeHumanVisualPose(target.visualYaw,target.scale,time,target.visualReaction,aliveHuman,body);
    const float attackT=target.attackTimer>0?1-clampf(target.attackTimer/HUMAN_SWING_ATTACK_DURATION,0.0f,1.0f):0;
    const float low=target.attackVariant>=2?1.0f:0.0f;
    const float reach=target.attackTimer>0?smoothStep01(clampf((attackT-HUMAN_SWING_COMMIT_PHASE)/(HUMAN_SWING_END_PHASE-HUMAN_SWING_COMMIT_PHASE),0.0f,1.0f)):0.0f;
    const Vec3 attackForward=lengthSq(target.attackDirection)>0.001f?normalized(target.attackDirection):Vec3{-std::sin(target.visualYaw),0,-std::cos(target.visualYaw)};
    const Quat rootQ=quaternionFromEulerXYZ(
        pose.torsoPitch,
        target.visualYaw+PI,
        pose.torsoRoll);
    const Vec3 attackLunge=attackForward*(target.attackTimer>0?reach*0.075f*target.scale:0.0f);
    const Vec3 root{target.pos.x+attackLunge.x,target.pos.y+(target.attackTimer>0?std::sin(attackT*PI)*0.024f*low:0),target.pos.z+attackLunge.z};
    const float matrix[16]={1-2*(rootQ.y*rootQ.y+rootQ.z*rootQ.z),2*(rootQ.x*rootQ.y+rootQ.z*rootQ.w),2*(rootQ.x*rootQ.z-rootQ.y*rootQ.w),0,2*(rootQ.x*rootQ.y-rootQ.z*rootQ.w),1-2*(rootQ.x*rootQ.x+rootQ.z*rootQ.z),2*(rootQ.y*rootQ.z+rootQ.x*rootQ.w),0,2*(rootQ.x*rootQ.z+rootQ.y*rootQ.w),2*(rootQ.y*rootQ.z-rootQ.x*rootQ.w),1-2*(rootQ.x*rootQ.x+rootQ.y*rootQ.y),0,0,0,0,1};
    // Model-file materials are deliberately not authoritative here: the source
    // asset is pale and made enemies read as unstyled mannequins. Gameplay
    // actors belong to Data's dark shell / bright signal value hierarchy.
    const VisualColor base=enemyShellColor(target.brute,target.attackVariant,0);const VisualColor damageColor=humanDamageSurfaceColor(base,setting,target.armor,target.brute?4.0f:2.0f,target.slurpable,target.hitFlash);
    const float searchSignal=reaction.searchAmount*reaction.awareness*(0.12f+reaction.uncertainty*0.10f);
    const float threatSignal=reaction.commitment*reaction.awareness*0.22f;
    const float fractureSignal=reaction.disruption*0.16f;
    const VisualColor signalColor{
        damageColor.r+(VisualIdentity::ElectricCyan.r-damageColor.r)*searchSignal+(VisualIdentity::ElectricMagenta.r-damageColor.r)*threatSignal+(VisualIdentity::WarmGold.r-damageColor.r)*fractureSignal,
        damageColor.g+(VisualIdentity::ElectricCyan.g-damageColor.g)*searchSignal+(VisualIdentity::ElectricMagenta.g-damageColor.g)*threatSignal+(VisualIdentity::WarmGold.g-damageColor.g)*fractureSignal,
        damageColor.b+(VisualIdentity::ElectricCyan.b-damageColor.b)*searchSignal+(VisualIdentity::ElectricMagenta.b-damageColor.b)*threatSignal+(VisualIdentity::WarmGold.b-damageColor.b)*fractureSignal};
    const bool parryCue=target.attackTimer>0&&attackT>=0.22f&&attackT<=0.46f;const float cue=parryCue?(0.10f+0.05f*std::sin(time*28.0f)):0.0f;const float cueColor[4]={signalColor.r+(0.55f-signalColor.r)*cue,signalColor.g+(0.96f-signalColor.g)*cue,signalColor.b+(1.0f-signalColor.b)*cue,humanModel_.color[3]};
    glPushMatrix();glTranslatef(root.x,root.y,root.z);glMultMatrixf(matrix);glScalef(pose.scale*pose.expressiveScale.x,pose.scale*pose.expressiveScale.y,pose.scale*pose.expressiveScale.z);if(shadow)glColor4f(0.012f,0.018f,0.022f,0.28f);else gradedColor(cueColor[0],cueColor[1],cueColor[2],cueColor[3]);glBegin(GL_TRIANGLES);
    const float thinning=humanShellThinningAmount(target.armor,target.brute?4.0f:2.0f,target.slurpable);
    for(std::size_t i=0;i+8<humanVertices_.size();i+=9){const std::size_t triangle=i/9;const Vec3 rawA{humanVertices_[i],humanVertices_[i+1],humanVertices_[i+2]},rawB{humanVertices_[i+3],humanVertices_[i+4],humanVertices_[i+5]},rawC{humanVertices_[i+6],humanVertices_[i+7],humanVertices_[i+8]},center=(rawA+rawB+rawC)*(1.0f/3.0f);if(humanShellTriangleMissingTowardCrit(triangle,thinning,center))continue;const Vec3 a=humanShellAbsorbTowardCrit(rawA,triangle,thinning),b=humanShellAbsorbTowardCrit(rawB,triangle,thinning),c=humanShellAbsorbTowardCrit(rawC,triangle,thinning);Vec3 n{};triangle_geometry::faceNormal(a,b,c,n);glNormal3f(n.x,n.y,n.z);glVertex3f(a.x,a.y,a.z);glVertex3f(b.x,b.y,b.z);glVertex3f(c.x,c.y,c.z);}glEnd();glPopMatrix();
}

void DesktopRenderer::drawEnemyReactionBillboard(const GameState& state,const TargetState& target){
    // Reactions belong to the combat body. Once the soul is exposed, the
    // capture silhouette must read cleanly without a surviving speech card.
    if(!target.alive||target.slurpable)return;
    const float hit=clampf(target.hitFlash,0.0f,1.0f),pull=clampf(target.vacuumPullAmount,0.0f,1.0f);
    const float activity=std::max(hit,pull*0.82f);
    if(activity<0.08f)return;
    constexpr const char* phrases[]={"RUDE","MY KNEES","WAIT","LETS GO","NOT THE PHONE","BEEP"};
    const unsigned hash=static_cast<unsigned>(std::abs(target.walkTargetSequence)*31+
        static_cast<int>(target.phase*997.0f)+target.attackVariant*17);
    const char* phrase=phrases[hash%6u];const int lengthChars=static_cast<int>(std::strlen(phrase));
    const float revealProgress=hit>pull?clampf((1.0f-hit)*2.6f+0.18f,0.0f,1.0f):clampf(pull*1.35f,0.0f,1.0f);
    const int visibleChars=std::max(1,std::min(lengthChars,static_cast<int>(std::ceil(lengthChars*revealProgress))));
    Vec3 right=normalized(cross3(state.camera.forward,{0,1,0}));if(lengthSq(right)<0.01f)right={1,0,0};
    const Vec3 up=normalized(cross3(right,state.camera.forward));
    const float beat=std::pow(std::max(0.0f,std::sin(state.time*18.0f+hash*0.17f)),4.0f);
    const float cameraDepth=std::max(0.25f,dot3(target.pos-state.camera.pos,state.camera.forward));
    const float depthScale=clampf(cameraDepth/4.0f,0.42f,1.0f);
    const float pixel=0.024f*depthScale*(1.0f+activity*0.18f),sx=1.0f+beat*0.22f,sy=1.0f-beat*0.16f;
    const float width=(visibleChars*6.0f-1.0f)*pixel*sx,height=7.0f*pixel*sy;
    const Vec3 center=target.pos+Vec3{0,(1.56f+activity*0.18f)*target.scale,0};
    glDisable(GL_LIGHTING);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
    glColor4f(0.018f,0.022f,0.027f,0.58f*activity);glBegin(GL_QUADS);
    const Vec3 padR=right*(width*0.56f),padU=up*(height*0.72f);
    for(const Vec3& p:{center-padR-padU,center+padR-padU,center+padR+padU,center-padR+padU})glVertex3f(p.x,p.y,p.z);
    glEnd();
    const Vec3 origin=center-right*(width*0.5f)+up*(height*0.5f);
    for(int index=0;index<visibleChars;++index){const auto rows=bitmapGlyph(phrase[index]);for(int row=0;row<7;++row)for(int col=0;col<5;++col)if(rows[row]&(1u<<(4-col))){
        const float x=(index*6.0f+col+0.5f)*pixel*sx,y=-(row+0.5f)*pixel*sy;
        const Vec3 p=origin+right*x+up*y,rx=right*(pixel*sx*0.48f),uy=up*(pixel*sy*0.48f);
        glColor4f(VisualIdentity::AcidChartreuse.r,VisualIdentity::AcidChartreuse.g,VisualIdentity::AcidChartreuse.b,0.82f+activity*0.18f);glBegin(GL_QUADS);
        for(const Vec3& q:{p-rx-uy,p+rx-uy,p+rx+uy,p-rx+uy})glVertex3f(q.x,q.y,q.z);glEnd();
    }}
    glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_LIGHTING);
}

void DesktopRenderer::drawSoulFlesh(const TargetState& target,const Vec3& center){
    auto index=[](int x,int y,int z){return x+y*3+z*9;};
    auto emitQuad=[&](int ia,int ib,int ic,int id){const Vec3 a=center+target.latticeSurfacePos[ia],b=center+target.latticeSurfacePos[ib],c=center+target.latticeSurfacePos[ic],d=center+target.latticeSurfacePos[id];Vec3 n{};triangle_geometry::faceNormal(a,b,c,n);glNormal3f(n.x,n.y,n.z);glVertex3f(a.x,a.y,a.z);glVertex3f(b.x,b.y,b.z);glVertex3f(c.x,c.y,c.z);triangle_geometry::faceNormal(a,c,d,n);glNormal3f(n.x,n.y,n.z);glVertex3f(a.x,a.y,a.z);glVertex3f(c.x,c.y,c.z);glVertex3f(d.x,d.y,d.z);};
    gradedColor(VisualIdentity::DigitalBlood.r,VisualIdentity::DigitalBlood.g,VisualIdentity::DigitalBlood.b);glBegin(GL_TRIANGLES);
    for(int y=0;y<2;++y)for(int z=0;z<2;++z){emitQuad(index(0,y,z),index(0,y+1,z),index(0,y+1,z+1),index(0,y,z+1));emitQuad(index(2,y,z),index(2,y,z+1),index(2,y+1,z+1),index(2,y+1,z));}
    for(int x=0;x<2;++x)for(int z=0;z<2;++z){emitQuad(index(x,0,z),index(x,0,z+1),index(x+1,0,z+1),index(x+1,0,z));emitQuad(index(x,2,z),index(x+1,2,z),index(x+1,2,z+1),index(x,2,z+1));}
    for(int x=0;x<2;++x)for(int y=0;y<2;++y){emitQuad(index(x,y,0),index(x+1,y,0),index(x+1,y+1,0),index(x,y+1,0));emitQuad(index(x,y,2),index(x,y+1,2),index(x+1,y+1,2),index(x+1,y,2));}
    glEnd();
    if(target.tetherVisible){const Vec3 base=target.tetherAnchor,apex=target.tetherDestination,axis=base-apex;const float len=length(axis);if(len>0.001f){const Vec3 forward=axis*(1.0f/len);const Vec3 reference=std::abs(forward.y)>0.92f?Vec3{1,0,0}:Vec3{0,1,0};const Vec3 right=normalized(cross3(forward,reference)),up=normalized(cross3(right,forward));const float activity=clampf(std::max(target.vacuumPullAmount,target.ingestProgress),0.0f,1.0f),radius=0.11f+target.tetherWidth*0.16f+activity*0.035f;constexpr int segments=20;glDisable(GL_LIGHTING);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glDepthMask(GL_FALSE);glBegin(GL_TRIANGLE_FAN);glColor4f(VisualIdentity::ElectricCyan.r,VisualIdentity::ElectricCyan.g,VisualIdentity::ElectricCyan.b,0.15f+activity*0.10f);glVertex3f(apex.x,apex.y,apex.z);for(int i=0;i<=segments;++i){const float angle=2.0f*PI*static_cast<float>(i)/segments;const Vec3 edge=base+right*(std::cos(angle)*radius)+up*(std::sin(angle)*radius);glColor4f(VisualIdentity::ElectricCyan.r,VisualIdentity::ElectricCyan.g,VisualIdentity::ElectricCyan.b,0.012f+activity*0.018f);glVertex3f(edge.x,edge.y,edge.z);}glEnd();glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_LIGHTING);}}
}

void DesktopRenderer::drawFieldGrass(int tileIndex,const VisualColor& tint) const{
    if(!fieldGrassTexture_){const auto pixels=field_grass_texture::pixels();glGenTextures(1,&fieldGrassTexture_);glBindTexture(GL_TEXTURE_2D,fieldGrassTexture_);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,field_grass_texture::Size,field_grass_texture::Size,0,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());}
    const auto material=semanticMaterial(SemanticSurface::FieldGround,VisualIdentity::FieldGround);
    const ScopedMaterial applied(material);
    const float z0=static_cast<float>(tileIndex)*ROOM_DEPTH,scale=material.textureWorldScale;
    glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,fieldGrassTexture_);glColor3f(tint.r,tint.g,tint.b);glNormal3f(0,1,0);glBegin(GL_QUADS);
    glTexCoord2f(0,0);glVertex3f(-ROOM_WIDTH*0.5f,0.003f,z0-ROOM_DEPTH*0.5f);glTexCoord2f(ROOM_WIDTH/scale,0);glVertex3f(ROOM_WIDTH*0.5f,0.003f,z0-ROOM_DEPTH*0.5f);glTexCoord2f(ROOM_WIDTH/scale,ROOM_DEPTH/scale);glVertex3f(ROOM_WIDTH*0.5f,0.003f,z0+ROOM_DEPTH*0.5f);glTexCoord2f(0,ROOM_DEPTH/scale);glVertex3f(-ROOM_WIDTH*0.5f,0.003f,z0+ROOM_DEPTH*0.5f);glEnd();glDisable(GL_TEXTURE_2D);
}

void DesktopRenderer::drawCityGround(int tileIndex) const{
    if(!citySurfaceTexture_){const auto pixels=city_surface_texture::pixels();glGenTextures(1,&citySurfaceTexture_);glBindTexture(GL_TEXTURE_2D,citySurfaceTexture_);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,city_surface_texture::Size,city_surface_texture::Size,0,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());}
    const auto material=semanticMaterial(SemanticSurface::CityAsphalt,render_contract::CityGround.baseColor);
    const ScopedMaterial applied(material);
    const float z0=static_cast<float>(tileIndex)*ROOM_DEPTH,scale=material.textureWorldScale;
    glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,citySurfaceTexture_);glColor3f(1,1,1);glNormal3f(0,1,0);glBegin(GL_QUADS);
    glTexCoord2f(0,0);glVertex3f(-ROOM_WIDTH*0.5f,0.003f,z0-ROOM_DEPTH*0.5f);glTexCoord2f(ROOM_WIDTH/scale,0);glVertex3f(ROOM_WIDTH*0.5f,0.003f,z0-ROOM_DEPTH*0.5f);glTexCoord2f(ROOM_WIDTH/scale,ROOM_DEPTH/scale);glVertex3f(ROOM_WIDTH*0.5f,0.003f,z0+ROOM_DEPTH*0.5f);glTexCoord2f(0,ROOM_DEPTH/scale);glVertex3f(-ROOM_WIDTH*0.5f,0.003f,z0+ROOM_DEPTH*0.5f);glEnd();glDisable(GL_TEXTURE_2D);
}

void DesktopRenderer::drawBirdFlocks(const GameState& state,int tileIndex,room_environment::RoomSetting setting) const{
    if(!birdSpriteTexture_||(setting!=room_environment::RoomSetting::Field&&setting!=room_environment::RoomSetting::Coastal))return;
    // The public-domain atlas contains authored idle, peck, hop, takeoff,
    // flight and landing rows. Deterministic local schedules make the birds
    // inhabitants of each room; nearby violence interrupts every schedule.
    const float tileZ=static_cast<float>(tileIndex)*ROOM_DEPTH;
    Vec3 cameraRight=normalized(cross3(state.camera.forward,{0,1,0}));
    if(lengthSq(cameraRight)<0.01f)cameraRight={1,0,0};
    const Vec3 cameraUp=normalized(cross3(cameraRight,state.camera.forward));
    glDisable(GL_LIGHTING);glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,birdSpriteTexture_);
    glEnable(GL_ALPHA_TEST);glAlphaFunc(GL_GREATER,0.08f);
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.88f,0.94f,0.96f,0.84f);
    const bool disturbed=state.environmentVisual.latestShotAge<1.8f||state.hud.criticalHitPulse>0.22f;
    for(int flock=0;flock<2;++flock){
        const float direction=flock==0?1.0f:-1.0f;
        const float localTime=std::fmod(state.time+static_cast<float>(tileIndex*7+flock*11)*1.73f,26.0f);
        float stateStart=0.0f,stateDuration=4.5f;
        bool airborne=false;
        if(disturbed){stateStart=0.0f;stateDuration=8.0f;airborne=true;}
        else if(localTime<8.0f){stateStart=0.0f;stateDuration=8.0f;}
        else if(localTime<10.5f){stateStart=8.0f;stateDuration=2.5f;}
        else if(localTime<12.0f){stateStart=10.5f;stateDuration=1.5f;airborne=true;}
        else if(localTime<23.0f){stateStart=12.0f;stateDuration=11.0f;airborne=true;}
        else {stateStart=23.0f;stateDuration=3.0f;}
        const float stateT=clampf((localTime-stateStart)/std::max(0.01f,stateDuration),0.0f,1.0f);
        for(int bird=0;bird<5;++bird){
            const float rank=static_cast<float>(bird),wing=bird==0?0.0f:static_cast<float>((bird+1)/2);
            const float side=bird==0?0.0f:(bird&1?1.0f:-1.0f);
            const float roostX=-3.7f+flock*6.4f+direction*wing*0.62f+side*0.18f;
            float x=roostX+direction*(!airborne?std::sin(stateT*PI*3.0f)*0.16f:0.0f);
            float y=0.24f+(!airborne?std::abs(std::sin(stateT*PI*3.0f))*0.10f:0.0f);
            float z=tileZ+(flock==0?-2.8f:3.0f)+side*wing*0.74f;
            if(airborne){
                const float flightT=disturbed?clampf(state.environmentVisual.latestShotAge/1.8f,0.0f,1.0f):stateT;
                x=roostX+direction*(flightT*9.0f-wing*0.58f);
                const float arc=disturbed?1.0f:std::sin(stateT*PI);
                y=0.30f+arc*(3.0f+flock*0.55f)-wing*0.07f+std::sin(state.time*0.9f+rank)*0.08f;
                z+=side*wing*0.22f;
            }
            const float halfW=0.34f+(bird%2)*0.025f,halfH=0.23f+(bird%3)*0.012f;
            const Vec3 center{x,y,z},rx=cameraRight*halfW,uy=cameraUp*halfH;
            const int frame=(static_cast<int>(std::floor(state.time*(airborne?10.0f:3.0f)+rank*1.7f+flock*2.0f)))%8;
            float u0=static_cast<float>(frame)/8.0f,u1=static_cast<float>(frame+1)/8.0f;
            if(direction<0.0f)std::swap(u0,u1);
            const float v0=0.0f,v1=1.0f;
            glBegin(GL_QUADS);
            glTexCoord2f(u0,v1);glVertex3f((center-rx-uy).x,(center-rx-uy).y,(center-rx-uy).z);
            glTexCoord2f(u1,v1);glVertex3f((center+rx-uy).x,(center+rx-uy).y,(center+rx-uy).z);
            glTexCoord2f(u1,v0);glVertex3f((center+rx+uy).x,(center+rx+uy).y,(center+rx+uy).z);
            glTexCoord2f(u0,v0);glVertex3f((center-rx+uy).x,(center-rx+uy).y,(center-rx+uy).z);
            glEnd();
        }
    }
    glDisable(GL_BLEND);glDisable(GL_ALPHA_TEST);glDisable(GL_TEXTURE_2D);glEnable(GL_LIGHTING);
}

void DesktopRenderer::drawFoliageCards(const Vec3& center,const Vec3& size,float yaw,const VisualColor& tint,unsigned int seed) const{
    if(!foliageTexture_){drawFacetedCanopy(center,size,yaw,tint,seed);return;}
    glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,foliageTexture_);
    glEnable(GL_ALPHA_TEST);glAlphaFunc(GL_GREATER,0.30f);
    glDisable(GL_CULL_FACE);glColor3f(tint.r,tint.g,tint.b);
    for(int plane=0;plane<3;++plane){
        const float angle=yaw+static_cast<float>(plane)*PI/3.0f;
        const Vec3 right{std::cos(angle)*size.x*0.5f,0,std::sin(angle)*size.z*0.5f};
        const Vec3 down{0,size.y*0.5f,0};
        const float uFlip=room_environment::unit(seed+static_cast<unsigned int>(plane)*17u)>0.5f?1.0f:0.0f;
        glBegin(GL_QUADS);glNormal3f(-std::sin(angle),0,std::cos(angle));
        glTexCoord2f(uFlip,1);glVertex3f(center.x-right.x-down.x,center.y-down.y,center.z-right.z-down.z);
        glTexCoord2f(1.0f-uFlip,1);glVertex3f(center.x+right.x-down.x,center.y-down.y,center.z+right.z-down.z);
        glTexCoord2f(1.0f-uFlip,0);glVertex3f(center.x+right.x+down.x,center.y+down.y,center.z+right.z+down.z);
        glTexCoord2f(uFlip,0);glVertex3f(center.x-right.x+down.x,center.y+down.y,center.z-right.z+down.z);glEnd();
    }
    glEnable(GL_CULL_FACE);glDisable(GL_ALPHA_TEST);glDisable(GL_TEXTURE_2D);
}

void DesktopRenderer::drawFieldBackdrop(const GameState& state,int tileIndex) const{
    if(!commonTreeModelList_||!twistedTreeModelList_||!deadTreeModelList_||!pineTreeModelList_)return;
    const float z0=static_cast<float>(tileIndex)*ROOM_DEPTH;
    const unsigned int key=room_environment::roomKey(state.roomSeed,state.roomIndex)^static_cast<unsigned int>((tileIndex+5)*0x9e3779b9u);
    struct Dressing { float x,z,height,yaw; unsigned int list; float widthCorrection; unsigned char family; };
    // Asymmetric edge groups frame the playable clearing. They are visual
    // history rather than obstacles, so the central route remains honest.
    const float driftA=(room_environment::unit(key+1u)-0.5f)*2.4f;
    const float driftB=(room_environment::unit(key+2u)-0.5f)*2.8f;
    const Dressing group[]{
        {-14.15f,-15.0f+driftA,7.6f, 0.10f,pineTreeModelList_,0.78f,0},
        {-13.65f, -7.4f+driftB,5.4f,-0.18f,commonTreeModelList_,0.94f,1},
        { 14.20f,  4.8f-driftA,6.7f, 0.36f,twistedTreeModelList_,0.82f,2},
        { 13.80f, 13.6f-driftB,8.4f,-0.12f,pineTreeModelList_,0.76f,0},
        {-13.95f, 15.7f+driftA,4.1f, 0.58f,deadTreeModelList_,0.88f,2},
    };
    glDisable(GL_LIGHTING);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
    for(const auto& tree:group){
        drawGroundShadow({tree.x,0,z0+tree.z},0.46f,0.34f,tree.height*0.34f,0.18f,0.0f);
    }
    glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_LIGHTING);
    int treeIndex=0;
    for(const auto& tree:group){
        const unsigned int treeKey=key+static_cast<unsigned int>((tree.z+20.0f)*17.0f)+static_cast<unsigned int>(treeIndex++)*97u;
        const float familyVariation=0.92f+room_environment::unit(treeKey)*0.16f;
        const float height=tree.height*familyVariation;
        const room_environment::EnvironmentPropSpec proxy{room_environment::EnvironmentPrimitive::Tree,room_environment::EnvironmentRole::Detail,{tree.x,0,z0+tree.z},{height*tree.widthCorrection,height,height*tree.widthCorrection},tree.yaw,tree.family};
        const auto variant=tree_geometry::authoredVariant(proxy,treeKey);
        const Vec3 base{tree.x,0.006f,z0+tree.z};
        drawStaticModel(tree.list,base,{height*tree.widthCorrection*variant.widthScale,height,height*tree.widthCorrection*variant.depthScale},quaternionFromEulerXYZ(variant.pitch,tree.yaw+variant.yawJitter,variant.roll));
        if(state.localSettings.graphicsPreset>0){
            const float rootRadius=height*(tree.family==1?0.028f:0.021f);
            const int roots=tree.family==1?5:3;
            for(int root=0;root<roots;++root){
                const float angle=tree.yaw+variant.yawJitter+static_cast<float>(root)*PI*2.0f/static_cast<float>(roots)+room_environment::unit(treeKey+static_cast<unsigned int>(root)*19u)*0.24f;
                const float reach=rootRadius*(tree.family==1?5.0f:4.0f)*variant.rootSpread;
                const Vec3 end=base+Vec3{std::cos(angle)*reach,0.016f,std::sin(angle)*reach};
                drawTaperedSegment(base+Vec3{0,rootRadius*0.30f,0},end,rootRadius*0.42f,0.014f,{0.27f,0.20f,0.13f},6);
            }
        }
    }
}

void DesktopRenderer::drawCityStreet(const GameState& state,int tileIndex,room_environment::RoomForm form) const{
    const float z0=static_cast<float>(tileIndex)*ROOM_DEPTH;
    const std::uint32_t key=room_environment::roomKey(state.roomSeed,state.roomIndex)^static_cast<std::uint32_t>((tileIndex+11)*0x9e3779b9u);
    const auto street=city_street::plan(form,key,state.localSettings.graphicsPreset);
    const float sidewalkCenter=street.roadHalfWidth+street.sidewalkWidth*0.5f;

    // The street section is one construction system: recessed asphalt, raised
    // pedestrian edges, a transverse intersection, gutters, and markings.
    drawTexturedBox(citySurfaceTexture_,{0,0.010f,z0},{street.roadHalfWidth*2.0f,0.022f,ROOM_DEPTH},0,0,0,{0.68f,0.70f,0.71f},2.15f,std::fmod((tileIndex+7)*0.173f,1.0f));
    for(float side:{-1.0f,1.0f}){
        drawBox({side*sidewalkCenter,0.050f,z0},{street.sidewalkWidth,0.10f,ROOM_DEPTH-0.28f},0,0,0,0.40f,0.405f,0.39f);
        drawBox({side*(street.roadHalfWidth+0.055f),0.082f,z0},{0.11f,0.164f,ROOM_DEPTH-0.20f},0,0,0,0.54f,0.53f,0.49f);
        drawBox({side*(street.roadHalfWidth-0.12f),0.022f,z0},{0.16f,0.024f,ROOM_DEPTH-0.36f},0,0,0,0.075f,0.085f,0.09f);
    }
    drawTexturedBox(citySurfaceTexture_,{0,0.025f,z0+street.intersectionCenter},{ROOM_WIDTH-0.7f,0.026f,street.intersectionWidth},0,0,0,{0.61f,0.63f,0.64f},2.15f,std::fmod((tileIndex+13)*0.127f,1.0f));
    for(int approach:{-1,1})for(int stripe=0;stripe<7;++stripe){
        const float x=-street.roadHalfWidth+0.58f+static_cast<float>(stripe)*(street.roadHalfWidth*2.0f-1.16f)/6.0f;
        const float z=z0+street.intersectionCenter+static_cast<float>(approach)*(street.intersectionWidth*0.5f+0.42f);
        drawBox({x,0.041f,z},{0.34f,0.014f,0.68f},0,0,0,0.72f,0.71f,0.64f);
    }
    for(int dash=0;dash<8;++dash){
        const float localZ=-17.5f+static_cast<float>(dash)*5.0f;
        if(std::abs(localZ-street.intersectionCenter)<street.intersectionWidth*0.62f)continue;
        const float wear=0.76f+0.12f*room_environment::unit(key+static_cast<std::uint32_t>(dash)*13u);
        drawBox({0,0.032f,z0+localZ},{0.075f,0.012f,1.75f},0,0,0,0.62f*wear,0.50f*wear,0.22f*wear);
    }
    // Drain covers repeat by civil-engineering rhythm rather than random
    // scatter. They sit in the gutter and visually explain water/wear.
    for(float side:{-1.0f,1.0f})for(int drain=0;drain<4;++drain){
        const float localZ=-14.5f+static_cast<float>(drain)*9.7f+(side>0?1.6f:0.0f);
        drawBox({side*(street.roadHalfWidth-0.16f),0.038f,z0+localZ},{0.28f,0.016f,0.62f},0,0,0,0.07f,0.085f,0.09f);
        for(int slot=0;slot<3;++slot)drawBox({side*(street.roadHalfWidth-0.16f),0.048f,z0+localZ-0.20f+slot*0.20f},{0.19f,0.006f,0.035f},0,0,0,0.20f,0.22f,0.22f);
    }

    // Optional inhabited-edge repetition scales with quality, while the street
    // section and skyline identity survive every preset.
    const float lampX=street.roadHalfWidth+street.sidewalkWidth-street.lampInset;
    for(float side:{-1.0f,1.0f})for(int lamp=0;lamp<street.lampPairs;++lamp){
        const float localZ=city_street::lampZ(street,lamp)+(side>0?1.1f:-1.1f);
        const float poleHeight=2.75f;
        drawBox({side*lampX,poleHeight*0.5f,z0+localZ},{0.075f,poleHeight,0.075f},0,0,0,0.13f,0.15f,0.16f);
        drawBox({side*(lampX-0.18f),poleHeight-0.04f,z0+localZ},{0.42f,0.065f,0.075f},0,0,0,0.13f,0.15f,0.16f);
        glDisable(GL_LIGHTING);drawBox({side*(lampX-0.37f),poleHeight-0.10f,z0+localZ},{0.16f,0.055f,0.11f},0,0,0,0.78f,0.71f,0.52f);glEnable(GL_LIGHTING);
    }
    for(int cluster=0;cluster<street.serviceClusters;++cluster){
        const float side=(cluster&1)?1.0f:-1.0f;
        const float localZ=-10.5f+cluster*19.0f+(room_environment::unit(key+81u+cluster)-0.5f)*2.0f;
        const float outer=street.roadHalfWidth+street.sidewalkWidth+0.46f;
        drawBox({side*outer,0.44f,z0+localZ},{0.62f,0.88f,0.44f},0,0,0,0.18f,0.21f,0.21f);
        drawBox({side*(outer-side*0.32f),0.50f,z0+localZ},{0.018f,0.52f,0.25f},0,0,0,0.075f,0.09f,0.095f);
        for(int vent=0;vent<3;++vent)drawBox({side*(outer-side*0.335f),0.61f-vent*0.13f,z0+localZ},{0.012f,0.025f,0.19f},0,0,0,0.36f,0.38f,0.36f);
        // A restrained service-status lamp makes the utility cluster readable
        // as street infrastructure rather than another anonymous grey box.
        glDisable(GL_LIGHTING);
        drawBox({side*(outer-side*0.342f),0.76f,z0+localZ},{0.014f,0.075f,0.12f},0,0,0,0.42f,0.68f,0.64f);
        glEnable(GL_LIGHTING);
    }

    for(int side:{-1,1})for(int index=0;index<street.skylineBands;++index){
        const auto block=city_street::skylineBlock(street,key,tileIndex,side,index);
        const VisualColor body=block.family==0?VisualColor{0.245f,0.26f,0.27f}:(block.family==1?VisualColor{0.31f,0.27f,0.235f}:VisualColor{0.20f,0.225f,0.245f});
        drawBox({block.x,block.height*0.5f,z0+block.z},{block.width,block.height,block.depth},0,0,0,body.r,body.g,body.b);
        drawBox({block.x,block.height+0.10f,z0+block.z},{block.width*1.04f,0.20f,block.depth*1.04f},0,0,0,body.r*0.78f,body.g*0.80f,body.b*0.82f);
        const float faceX=block.x-static_cast<float>(side)*block.width*0.505f;
        const int rows=state.localSettings.graphicsPreset<=0?2:3;
        const unsigned int facadeKey=key^static_cast<unsigned int>((side+2)*193+(index+1)*977);
        const auto atmosphereInputs=render_contract::effectiveAtmosphereInputs(lightingControl_,{state.time,state.roomIndex,state.vacuum.power*0.62f});
        const float nightPresence=0.34f+(1.0f-render_contract::celestialDaylight(atmosphereInputs.time))*0.66f;
        for(int row=0;row<rows;++row)for(int bay=0;bay<2;++bay){
            const float y=1.4f+row*(block.height-2.0f)/static_cast<float>(std::max(1,rows-1));
            const float z=z0+block.z+(bay?0.22f:-0.22f)*block.depth;
            const bool occupied=city_facade::windowOccupied(facadeKey,row,bay);
            if(occupied){
                const float warmth=city_facade::occupiedWindowWarmth(facadeKey,row,bay);
                glDisable(GL_LIGHTING);
                drawBox({faceX,y,z},{0.025f,0.34f,block.depth*0.26f},0,0,0,(0.58f+warmth*0.18f)*nightPresence,(0.34f+warmth*0.12f)*nightPresence,(0.13f+warmth*0.05f)*nightPresence);
                glEnable(GL_LIGHTING);
            }else drawBox({faceX,y,z},{0.025f,0.34f,block.depth*0.26f},0,0,0,0.025f,0.038f,0.045f);
        }
    }
}

void DesktopRenderer::drawFacetedRock(const room_environment::EnvironmentPropSpec& prop,int roomSeed,int roomIndex,int propIndex,float zOffset,const VisualColor& color,unsigned int texture,float textureWorldScale){
    const auto mesh=faceted_rock::makeVisualMesh(prop,roomSeed,roomIndex,propIndex,zOffset);
    if(texture){glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture);}
    gradedColor(color.r,color.g,color.b);glBegin(GL_TRIANGLES);
    for(int i=0;i<mesh.vertexCount;++i){
        const float x=mesh.positions[i*3],y=mesh.positions[i*3+1],z=mesh.positions[i*3+2];
        const float nx=mesh.normals[i*3],ny=mesh.normals[i*3+1],nz=mesh.normals[i*3+2];
        glNormal3f(nx,ny,nz);
        if(texture){const float ax=std::abs(nx),ay=std::abs(ny),az=std::abs(nz);if(ay>=ax&&ay>=az)glTexCoord2f(x/textureWorldScale,z/textureWorldScale);else if(ax>=az)glTexCoord2f(z/textureWorldScale,y/textureWorldScale);else glTexCoord2f(x/textureWorldScale,y/textureWorldScale);}
        glVertex3f(x,y,z);
    }
    glEnd();if(texture)glDisable(GL_TEXTURE_2D);
}

void DesktopRenderer::drawTaperedSegment(const Vec3& start,const Vec3& end,float startRadius,float endRadius,const VisualColor& color,int sides){
    const Vec3 axis=end-start;const float axisLength=length(axis);if(axisLength<0.001f)return;
    const Vec3 up=axis*(1.0f/axisLength),reference=std::abs(up.y)>0.92f?Vec3{1,0,0}:Vec3{0,1,0};
    const Vec3 right=normalized(cross3(up,reference)),forward=normalized(cross3(right,up));
    gradedColor(color.r,color.g,color.b);glBegin(GL_TRIANGLES);
    for(int side=0;side<sides;++side){
        const float a0=2.0f*PI*static_cast<float>(side)/sides,a1=2.0f*PI*static_cast<float>(side+1)/sides;
        const Vec3 radial0=right*std::cos(a0)+forward*std::sin(a0),radial1=right*std::cos(a1)+forward*std::sin(a1);
        const Vec3 b0=start+radial0*startRadius,b1=start+radial1*startRadius,t0=end+radial0*endRadius,t1=end+radial1*endRadius;
        const Vec3 n0=normalized(radial0*axisLength-up*(endRadius-startRadius)),n1=normalized(radial1*axisLength-up*(endRadius-startRadius));
        glNormal3f(n0.x,n0.y,n0.z);glVertex3f(b0.x,b0.y,b0.z);glNormal3f(n1.x,n1.y,n1.z);glVertex3f(b1.x,b1.y,b1.z);glNormal3f(n1.x,n1.y,n1.z);glVertex3f(t1.x,t1.y,t1.z);
        glNormal3f(n0.x,n0.y,n0.z);glVertex3f(b0.x,b0.y,b0.z);glNormal3f(n1.x,n1.y,n1.z);glVertex3f(t1.x,t1.y,t1.z);glNormal3f(n0.x,n0.y,n0.z);glVertex3f(t0.x,t0.y,t0.z);
        glNormal3f(-up.x,-up.y,-up.z);glVertex3f(start.x,start.y,start.z);glVertex3f(b1.x,b1.y,b1.z);glVertex3f(b0.x,b0.y,b0.z);
        glNormal3f(up.x,up.y,up.z);glVertex3f(end.x,end.y,end.z);glVertex3f(t0.x,t0.y,t0.z);glVertex3f(t1.x,t1.y,t1.z);
    }
    glEnd();
}

void DesktopRenderer::drawFacetedCanopy(const Vec3& center,const Vec3& size,float yaw,const VisualColor& color,unsigned int seed){
    constexpr int sides=8;const float phase=yaw+static_cast<float>(seed&7u)*0.071f;
    const auto point=[&](int ring,int side){
        const float angle=phase+2.0f*PI*static_cast<float>(side)/sides;
        const float irregular=0.88f+0.12f*room_environment::unit(seed+static_cast<unsigned int>(ring*19+side*7));
        const float ringRadius=ring==0?0.40f:(ring==1?0.52f:0.35f),ringY=ring==0?-0.22f:(ring==1?0.08f:0.30f);
        return center+Vec3{std::cos(angle)*size.x*ringRadius*irregular,size.y*ringY,std::sin(angle)*size.z*ringRadius*irregular};
    };
    const Vec3 bottom=center-Vec3{0,size.y*0.42f,0},top=center+Vec3{0,size.y*0.48f,0};
    gradedColor(color.r,color.g,color.b);glBegin(GL_TRIANGLES);
    const auto emit=[&](const Vec3& a,const Vec3& b,const Vec3& c){const Vec3 n=triangle_geometry::faceNormalOrZero(a,b,c);glNormal3f(n.x,n.y,n.z);glVertex3f(a.x,a.y,a.z);glVertex3f(b.x,b.y,b.z);glVertex3f(c.x,c.y,c.z);};
    for(int side=0;side<sides;++side){const int next=(side+1)%sides;const Vec3 a=point(0,side),b=point(0,next),c=point(1,side),d=point(1,next),e=point(2,side),f=point(2,next);emit(bottom,b,a);emit(a,b,d);emit(a,d,c);emit(c,d,f);emit(c,f,e);emit(e,f,top);}
    glEnd();
}

void DesktopRenderer::drawSlopeWedge(const SlopeSupport& slope,float zOffset,const VisualColor& color){
    const auto mesh=makeSlopeWedgeMesh(slope,zOffset);gradedColor(color.r,color.g,color.b);glBegin(GL_TRIANGLES);
    for(int i=0;i<mesh.vertexCount;++i){glNormal3f(mesh.normals[i*3],mesh.normals[i*3+1],mesh.normals[i*3+2]);glVertex3f(mesh.positions[i*3],mesh.positions[i*3+1],mesh.positions[i*3+2]);}glEnd();
}

void DesktopRenderer::updateCoastalWater(const GameState& state) const{
    const auto plan=room_environment::roomPlan(state.roomSeed,state.roomIndex);
    if(plan.setting!=room_environment::RoomSetting::Coastal){coastalWaterTime_=state.time;return;}
    const bool changed=coastalWaterRoom_!=state.roomIndex||coastalWaterSeed_!=state.roomSeed||coastalWaterTime_<0.0f||state.time<coastalWaterTime_;
    if(changed){coastalWaterSimulation_.reset();coastalWaterRoom_=state.roomIndex;coastalWaterSeed_=state.roomSeed;coastalWaterTime_=state.time;coastalWaterPreviousPlayer_=state.player.pos;return;}
    const float elapsed=std::min(0.10f,std::max(0.0f,state.time-coastalWaterTime_));
    if(elapsed<=0.000001f)return;
    const int steps=std::max(1,std::min(8,static_cast<int>(std::ceil(elapsed/(1.0f/120.0f)))));
    for(int step=0;step<steps;++step)coastalWaterSimulation_.step(elapsed/static_cast<float>(steps));
    const float tileOrigin=static_cast<float>(state.topology.currentTileIndex)*ROOM_DEPTH;
    const float localZ=state.player.pos.z-tileOrigin;
    if(coastal_water::contains(state.player.pos.x,localZ)){
        const float speed=std::sqrt(state.player.vel.x*state.player.vel.x+state.player.vel.z*state.player.vel.z);
        const float movement=std::sqrt((state.player.pos.x-coastalWaterPreviousPlayer_.x)*(state.player.pos.x-coastalWaterPreviousPlayer_.x)+(state.player.pos.z-coastalWaterPreviousPlayer_.z)*(state.player.pos.z-coastalWaterPreviousPlayer_.z));
        const float strength=-(0.10f+std::min(1.0f,speed/10.0f)*0.24f)*std::min(1.0f,elapsed*60.0f);
        coastalWaterSimulation_.disturb(state.player.pos.x,localZ,strength,0.48f+std::min(0.55f,movement*2.5f));
        if(speed>1.0f){const float inv=1.0f/std::max(0.001f,speed);coastalWaterSimulation_.disturb(state.player.pos.x-state.player.vel.x*inv*0.44f,localZ-state.player.vel.z*inv*0.44f,-strength*0.52f,0.52f);}
    }
    coastalWaterPreviousPlayer_=state.player.pos;
    coastalWaterTime_=state.time;
}

void DesktopRenderer::drawRoomTile(const GameState& state,int tileIndex,const scene_lighting_response::Response& lightingResponse) const {
    const auto plan=room_environment::roomPlan(state.roomSeed,state.roomIndex);
    const auto lightRig=room_lighting::roomLightRig(plan.setting,plan.form);
    const float z0 = static_cast<float>(tileIndex) * ROOM_DEPTH;
    const float doorWidth = 5.35f;
    const float doorHeight = 3.95f;
    const float sideW = (ROOM_WIDTH - doorWidth) * 0.5f;
    const float sideX = doorWidth * 0.5f + sideW * 0.5f;
    const float topH = ROOM_WALL_HEIGHT - doorHeight;
    const float topY = doorHeight + topH * 0.5f;
    const float wallR = VisualIdentity::RoomWall.r, wallG = VisualIdentity::RoomWall.g, wallB = VisualIdentity::RoomWall.b;
    const bool field=plan.setting==room_environment::RoomSetting::Field,city=plan.setting==room_environment::RoomSetting::City,sterile=plan.setting==room_environment::RoomSetting::Sterile,coastal=plan.setting==room_environment::RoomSetting::Coastal;
    const bool openNatural=(field&&plan.form==room_environment::RoomForm::Open)||coastal;
    const std::uint32_t realityKey=room_environment::roomKey(state.roomSeed,state.roomIndex);
    const bool mutationEligible=state.roomIndex>=8&&!state.roomInspector;
    const auto& permanent=state.progression.permanent;
    const bool wireframeUnlocked=material_unlocks::unlocked(material_unlocks::Memory::Wireframe,permanent.lifetimeRoomsCleared,permanent.bestRoomCleared);
    const bool gummyUnlocked=material_unlocks::unlocked(material_unlocks::Memory::Gummy,permanent.lifetimeRoomsCleared,permanent.bestRoomCleared);
    const bool festiveUnlocked=material_unlocks::unlocked(material_unlocks::Memory::Festive,permanent.lifetimeRoomsCleared,permanent.bestRoomCleared);
    const bool wireframeMutation=mutationEligible&&wireframeUnlocked&&room_environment::mix(realityKey+0x71u)%43u==0u;
    const bool gummyMutation=mutationEligible&&gummyUnlocked&&!wireframeMutation&&room_environment::mix(realityKey+0x93u)%47u==0u;
    const bool festiveMutation=mutationEligible&&festiveUnlocked&&!wireframeMutation&&!gummyMutation&&room_environment::mix(realityKey+0xb5u)%53u==0u;
    const auto drawStructuralBox=[&](const Vec3& center,const Vec3& size,float pitch,float yaw,float roll,float r,float g,float b){
        if(sterile){const ScopedMaterial material(semanticMaterial(SemanticSurface::SterilePanel,{r,g,b}));drawTexturedBox(sterileTileTexture_,center,size,pitch,yaw,roll,{r*0.82f,g*0.86f,b*0.88f},1.34f,std::fmod(std::abs(center.x*0.071f+center.z*0.037f),1.0f));}
        else drawBox(center,size,pitch,yaw,roll,r,g,b);
    };
    drawBox({0,-0.04f,z0},{openNatural?ROOM_WIDTH*2.8f:ROOM_WIDTH,0.08f,ROOM_DEPTH},0,0,0,field?VisualIdentity::FieldGround.r:(sterile?0.58f:(coastal?0.24f:VisualIdentity::RoomFloor.r)),field?VisualIdentity::FieldGround.g:(sterile?0.61f:(coastal?0.43f:VisualIdentity::RoomFloor.g)),field?VisualIdentity::FieldGround.b:(sterile?0.63f:(coastal?0.50f:VisualIdentity::RoomFloor.b)));
    if(field&&plan.form==room_environment::RoomForm::Open){
        if(festiveMutation||gummyMutation)drawTexturedBox(sandTexture_,{0,0.005f,z0},{ROOM_WIDTH-0.55f,0.018f,ROOM_DEPTH-0.55f},0,0,0,festiveMutation?VisualColor{0.86f,0.94f,0.98f}:VisualColor{1.0f,0.46f,0.86f},2.8f,std::fmod(state.roomIndex*0.173f,1.0f));
        else drawFieldGrass(tileIndex);
    }
    if(city){drawCityGround(tileIndex);drawCityStreet(state,tileIndex,plan.form);}
    if(coastal)drawTexturedBox(sandTexture_,{0,0.005f,z0},{23.5f,0.018f,35.5f},0,0,0,festiveMutation?VisualColor{0.80f,0.88f,0.92f}:VisualColor{0.91f,0.84f,0.68f},2.8f,std::fmod(state.roomIndex*0.173f,1.0f));
    if(sterile)drawTexturedBox(sterileTileTexture_,{0,0.006f,z0},{ROOM_WIDTH-0.55f,0.016f,ROOM_DEPTH-0.55f},0,0,0,{0.76f,0.79f,0.78f},1.34f,std::fmod(state.roomIndex*0.113f,1.0f));
    if(sterile){
        drawStructuralBox({0,ROOM_WALL_HEIGHT+0.08f,z0},{ROOM_WIDTH,0.16f,ROOM_DEPTH},0,0,0,wallR,wallG,wallB);
        glDisable(GL_LIGHTING);
        for(int i=0;i<lightRig.localLightCount;++i){const auto& fixture=lightRig.localLights[i];if(fixture.visibleFixture)drawBox(fixture.localPosition+Vec3{0,0,z0},fixture.fixtureSize,0,0,0,fixture.color.r*fixture.intensity,fixture.color.g*fixture.intensity,fixture.color.b*fixture.intensity);}
        glEnable(GL_LIGHTING);
    }
    if(openNatural){
        for(float seam:{-ROOM_DEPTH*0.5f,ROOM_DEPTH*0.5f}){
            const VisualColor frame=field?VisualColor{0.20f,0.27f,0.22f}:VisualColor{0.33f,0.36f,0.34f};
            drawBox({-doorWidth*0.56f,doorHeight*0.48f,z0+seam},{0.24f,doorHeight*0.96f,0.28f},0,0,0,frame.r,frame.g,frame.b);
            drawBox({ doorWidth*0.56f,doorHeight*0.48f,z0+seam},{0.24f,doorHeight*0.96f,0.28f},0,0,0,frame.r,frame.g,frame.b);
            drawBox({0,doorHeight*0.96f,z0+seam},{doorWidth*1.16f,0.24f,0.28f},0,0,0,frame.r,frame.g,frame.b);
        }
        if(field)drawFieldBackdrop(state,tileIndex);
    }else if(city){
        // City is a continuous exterior world. Buildings and the street section
        // define its edges; reusing the Sterile room shell created the former
        // giant terminal wall and destroyed urban depth.
    }else{
        for (float seam : {-ROOM_DEPTH*0.5f, ROOM_DEPTH*0.5f}) {
            drawStructuralBox({-sideX,ROOM_WALL_HEIGHT*0.5f,z0+seam},{sideW,ROOM_WALL_HEIGHT,0.5f},0,0,0,wallR,wallG,wallB);
            drawStructuralBox({ sideX,ROOM_WALL_HEIGHT*0.5f,z0+seam},{sideW,ROOM_WALL_HEIGHT,0.5f},0,0,0,wallR,wallG,wallB);
            drawStructuralBox({0,topY,z0+seam},{doorWidth,topH,0.5f},0,0,0,wallR,wallG,wallB);
        }
        drawStructuralBox({-ROOM_WIDTH*0.5f,ROOM_WALL_HEIGHT*0.5f,z0},{0.5f,ROOM_WALL_HEIGHT,ROOM_DEPTH},0,0,0,wallR,wallG,wallB);
        drawStructuralBox({ ROOM_WIDTH*0.5f,ROOM_WALL_HEIGHT*0.5f,z0},{0.5f,ROOM_WALL_HEIGHT,ROOM_DEPTH},0,0,0,wallR,wallG,wallB);
    }
    // Render-only mid-scale composition. These pieces break up room-sized slabs
    // without changing collision, traversal, or the authored capacity plan.
    if(field){
        const float boundaryX=ROOM_WIDTH*0.5f-0.42f;
        for(int side=-1;side<=1;side+=2)for(int marker=0;marker<5;++marker){
            const float localZ=-ROOM_DEPTH*0.38f+static_cast<float>(marker)*ROOM_DEPTH*0.19f;
            const float height=0.48f+0.16f*std::sin(static_cast<float>(tileIndex*7+marker*3+side));
            drawBox({side*boundaryX,height*0.5f,z0+localZ},{0.09f,height,0.09f},0,0,side*0.025f,0.20f,0.31f,0.19f);
        }
    }else if(sterile){
        for(int side=-1;side<=1;side+=2){
            for(int bay=0;bay<5;++bay){
                const float localZ=-ROOM_DEPTH*0.40f+bay*ROOM_DEPTH*0.20f;
                drawStructuralBox({side*(ROOM_WIDTH*0.5f-0.24f),ROOM_WALL_HEIGHT*0.47f,z0+localZ},{0.035f,ROOM_WALL_HEIGHT*0.62f,ROOM_DEPTH*0.0028f},0,0,0,0.32f,0.39f,0.42f);
            }
            glDisable(GL_LIGHTING);drawBox({side*(ROOM_WIDTH*0.5f-0.215f),0.42f,z0},{0.028f,0.055f,ROOM_DEPTH-1.0f},0,0,0,0.30f,0.72f,0.78f);glEnable(GL_LIGHTING);
        }
    }else if(coastal){
        const int strips=state.localSettings.graphicsPreset<=0?12:(state.localSettings.graphicsPreset==1?18:24);
        const int steps=state.localSettings.graphicsPreset<=0?24:(state.localSettings.graphicsPreset==1?36:48);
        const float x0=coastal_water::ShoreX,x1=coastal_water::FarX,zMin=z0+coastal_water::MinZ,zMax=z0+coastal_water::MaxZ;
        // The existing Ground054 sand becomes darker and finer where it is
        // saturated. Reusing one source material keeps the shore coherent.
        drawTexturedBox(sandTexture_,{x0-0.34f,0.026f,z0},{0.86f,0.018f,ROOM_DEPTH*0.98f},0,0,0,{0.64f,0.60f,0.49f},1.65f,std::fmod((state.roomIndex+tileIndex)*0.217f,1.0f));
        const float playerLocalZ=state.player.pos.z-z0;
        const float playerSpeed=std::sqrt(state.player.vel.x*state.player.vel.x+state.player.vel.z*state.player.vel.z);
        const bool playerInThisWater=tileIndex==state.topology.currentTileIndex&&coastal_water::contains(state.player.pos.x,playerLocalZ);
        const auto waterHeight=[&](float x,float localZ){return coastal_water::surfaceHeight(x,localZ,state.time)+(tileIndex==state.topology.currentTileIndex?coastalWaterSimulation_.sample(x,localZ):0.0f);};
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);glBegin(GL_TRIANGLES);
        const auto waterVertex=[&](float x,float z){const float localZ=z-z0;constexpr float epsilon=0.055f;const float y=waterHeight(x,localZ);const Vec3 normal=normalized(Vec3{waterHeight(x-epsilon,localZ)-waterHeight(x+epsilon,localZ),epsilon*2.0f,waterHeight(x,localZ-epsilon)-waterHeight(x,localZ+epsilon)});glNormal3f(normal.x,normal.y,normal.z);const float shore=coastal_water::shoreAmount(x);const float crest=clampf((y-0.025f)*4.8f,0.0f,0.22f);glColor4f(0.035f+shore*0.025f+crest,0.30f+shore*0.17f+crest,0.43f+shore*0.23f+crest,0.74f+shore*0.12f);glVertex3f(x,y,z);};
        for(int ix=0;ix<strips;++ix)for(int iz=0;iz<steps;++iz){const float xa=x0+(x1-x0)*ix/strips,xb=x0+(x1-x0)*(ix+1)/strips,za=zMin+(zMax-zMin)*iz/steps,zb=zMin+(zMax-zMin)*(iz+1)/steps;waterVertex(xa,za);waterVertex(xb,za);waterVertex(xb,zb);waterVertex(xa,za);waterVertex(xb,zb);waterVertex(xa,zb);}glEnd();
        glDisable(GL_LIGHTING);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glBegin(GL_LINES);for(int foam=0;foam<4;++foam){const float x=x0+0.12f+foam*0.36f+std::sin(state.time*0.64f+foam*1.7f+tileIndex)*0.10f;glColor4f(0.72f,0.92f,0.96f,0.34f-foam*0.045f);for(int iz=0;iz<steps;++iz){const float za=zMin+(zMax-zMin)*iz/steps,zb=zMin+(zMax-zMin)*(iz+1)/steps;const float waveXa=x+std::sin(za*0.55f+state.time)*0.035f,waveXb=x+std::sin(zb*0.55f+state.time)*0.035f;glVertex3f(waveXa,waterHeight(waveXa,za-z0)+0.012f,za);glVertex3f(waveXb,waterHeight(waveXb,zb-z0)+0.012f,zb);}}glEnd();
        if(playerInThisWater){
            const float speedAmount=clampf(playerSpeed/8.0f,0.0f,1.0f);glLineWidth(1.0f+speedAmount*1.5f);glBegin(GL_LINES);
            for(int ring=0;ring<3;++ring){const float age=std::fmod(state.time*1.35f+ring/3.0f,1.0f),radius=0.18f+age*(0.58f+speedAmount*0.52f);glColor4f(0.62f,0.92f,1.0f,(1.0f-age)*(0.09f+speedAmount*0.25f));for(int segment=0;segment<24;++segment){if((segment+ring*5)%8==0)continue;const float a=2.0f*PI*segment/24.0f,b=2.0f*PI*(segment+1)/24.0f;const float rippleA=radius*(1.0f+0.045f*std::sin(a*3.0f+state.time*2.1f+ring)),rippleB=radius*(1.0f+0.045f*std::sin(b*3.0f+state.time*2.1f+ring));const float ax=state.player.pos.x+std::cos(a)*rippleA,az=playerLocalZ+std::sin(a)*rippleA*0.58f,bx=state.player.pos.x+std::cos(b)*rippleB,bz=playerLocalZ+std::sin(b)*rippleB*0.58f;glVertex3f(ax,waterHeight(ax,az)+0.014f,z0+az);glVertex3f(bx,waterHeight(bx,bz)+0.014f,z0+bz);}}glEnd();glLineWidth(1.0f);
        }
        glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_LIGHTING);
    }
    if(tileIndex==state.topology.currentTileIndex&&lightingResponse.exitGlow>0.01f){
        const VisualColor guide=sterile?VisualColor{0.62f,0.88f,0.94f}:VisualColor{0.72f,0.90f,0.82f};
        glDisable(GL_LIGHTING);drawBox({0,doorHeight+0.12f,z0-ROOM_DEPTH*0.5f+0.27f},{doorWidth,0.12f,0.08f},0,0,0,guide.r*(0.38f+lightingResponse.exitGlow*0.62f),guide.g*(0.38f+lightingResponse.exitGlow*0.62f),guide.b*(0.38f+lightingResponse.exitGlow*0.62f));glEnable(GL_LIGHTING);
    }
    const int authoredObstacleCount=(state.traversalLab||state.slopeLab)?state.debug.colliderCount:std::min(state.debug.colliderCount,plan.obstacleCount);
    for (int i=0;i<authoredObstacleCount;++i) {
        const RoomCollider& c=state.roomColliders[i];
        const float materialPhase=0.5f+0.5f*std::sin(static_cast<float>(state.roomSeed*3+state.roomIndex*11+i*17));
        const VisualColor substrate=roomSubstrateColor(plan.setting);
        const VisualColor obstacleColor={
            VisualIdentity::RoomObstacle.r*0.72f+substrate.r*(0.20f+materialPhase*0.08f),
            VisualIdentity::RoomObstacle.g*0.72f+substrate.g*(0.20f+materialPhase*0.08f),
            VisualIdentity::RoomObstacle.b*0.72f+substrate.b*(0.20f+materialPhase*0.08f)};
        const unsigned int obstacleTexture=sterile?sterileTileTexture_:(plan.setting==room_environment::RoomSetting::City?metalTexture_:stoneTexture_);
        const VisualColor basePhotoTint=sterile?VisualColor{0.72f,0.75f,0.74f}:
            (plan.setting==room_environment::RoomSetting::City?VisualColor{0.68f,0.72f,0.75f}:
            (coastal?VisualColor{0.82f,0.80f,0.68f}:VisualColor{0.74f,0.78f,0.68f}));
        // Related surfaces share a material family without sharing one exact
        // swatch. The deterministic warm/cool drift avoids a patchwork look
        // while breaking up the old room-wide color uniformity.
        const float warm=0.92f+materialPhase*0.14f,cool=1.06f-materialPhase*0.11f,green=0.96f+std::sin(materialPhase*6.28318f)*0.045f;
        const VisualColor variedPhotoTint={basePhotoTint.r*warm,basePhotoTint.g*green,basePhotoTint.b*cool};
        const VisualColor photoTint=wireframeMutation?VisualColor{0.12f,0.15f,0.18f}:(gummyMutation?(i&1?VisualColor{0.98f,0.42f,0.72f}:VisualColor{0.36f,0.90f,0.82f}):variedPhotoTint);
        const bool coastalRock=coastal&&!wireframeMutation&&!gummyMutation;
        if(plan.setting==room_environment::RoomSetting::City){
            const VisualColor plaster=(i%3)==0?VisualColor{0.40f,0.38f,0.34f}:((i%3)==1?VisualColor{0.34f,0.37f,0.38f}:VisualColor{0.42f,0.36f,0.33f});
            drawBox({c.center.x,c.center.y,z0+c.center.z},{c.width,c.height,c.depth},0,0,0,plaster.r,plaster.g,plaster.b);
        }else if(coastalRock){
            const auto role=room_environment::obstacleRole(plan,state.roomSeed,state.roomIndex,i);
            const room_environment::EnvironmentPropSpec rock{room_environment::EnvironmentPrimitive::Rock,role,{c.center.x,c.bottomY,c.center.z},{c.width,c.height/0.71f,c.depth},materialPhase*0.34f,static_cast<unsigned char>(i%3)};
            drawFacetedRock(rock,state.roomSeed,state.roomIndex,i,z0,photoTint,stoneTexture_,1.35f);
        }else drawTexturedBox(obstacleTexture,{c.center.x,c.center.y,z0+c.center.z},{c.width,c.height,c.depth},0,0,0,photoTint,1.8f,materialPhase);
        if(wireframeMutation){glDisable(GL_LIGHTING);glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);drawBox({c.center.x,c.center.y,z0+c.center.z},{c.width+0.018f,c.height+0.018f,c.depth+0.018f},0,0,0,(i&1)?0.92f:0.18f,0.24f,(i&1)?0.78f:0.94f);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);glEnable(GL_LIGHTING);}
        if(gummyMutation){const float pulse=0.015f+0.012f*std::sin(state.time*2.2f+i);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);drawBox({c.center.x,c.center.y,z0+c.center.z},{c.width+pulse,c.height+pulse,c.depth+pulse},0,0,0,(i&1)?0.98f:0.28f,(i&1)?0.34f:0.92f,(i&1)?0.66f:0.78f,0.16f);glDepthMask(GL_TRUE);glDisable(GL_BLEND);}
        // A shallow dark plinth provides contact and a worn cap catches the
        // directional light. Both stay inside the authored collider footprint.
        if(!coastalRock){
            drawBox({c.center.x,0.022f,z0+c.center.z},{c.width*0.94f,0.035f,c.depth*0.94f},0,0,0,obstacleColor.r*0.34f,obstacleColor.g*0.36f,obstacleColor.b*0.38f);
            drawBox({c.center.x,c.topY+0.012f,z0+c.center.z},{c.width*0.88f,0.024f,c.depth*0.88f},0,0,0,std::min(1.0f,obstacleColor.r*1.18f),std::min(1.0f,obstacleColor.g*1.16f),std::min(1.0f,obstacleColor.b*1.14f));
        }
        if(festiveMutation){drawBox({c.center.x,c.topY+0.042f,z0+c.center.z},{c.width*0.94f,0.055f,c.depth*0.94f},0,0,0,0.88f,0.94f,0.96f);drawBox({c.center.x,c.center.y,z0+c.center.z-c.depth*0.505f},{c.width*(0.22f+0.05f*(i%3)),c.height*0.12f,0.025f},0,0,0,(i&1)?0.70f:0.12f,(i&1)?0.10f:0.58f,0.12f);}
        if(plan.setting==room_environment::RoomSetting::Sterile){
            const float faceZ=z0+c.center.z-c.depth*0.5f-0.014f;
            drawBox({c.center.x,c.center.y,faceZ},{c.width*0.64f,c.height*0.52f,0.022f},0,0,0,obstacleColor.r*0.66f,obstacleColor.g*0.72f,obstacleColor.b*0.76f);
            glDisable(GL_LIGHTING);drawBox({c.center.x,c.center.y+c.height*0.22f,faceZ-0.014f},{c.width*0.46f,0.025f,0.012f},0,0,0,0.28f,0.70f,0.76f);glEnable(GL_LIGHTING);
            for(float side:{-1.0f,1.0f})drawBox({c.center.x+side*c.width*0.43f,c.center.y,faceZ-0.022f},{0.045f,c.height*0.82f,0.035f},0,0,0,0.20f,0.24f,0.25f);
            if(c.height>1.25f){for(float side:{-1.0f,1.0f})drawBox({c.center.x+side*c.width*0.32f,0.075f,z0+c.center.z},{0.12f,0.15f,c.depth*0.64f},0,0,0,0.18f,0.21f,0.21f);}
        }else if(plan.setting==room_environment::RoomSetting::City){
            // Buildings line the sides of the route, so their authored facade
            // faces inward along X.  The old decoration was placed on -Z and
            // was practically invisible during play.
            const float inward=c.center.x>0.0f?-1.0f:1.0f;
            const float faceX=c.center.x+inward*(c.width*0.5f+0.014f);
            const unsigned int facadeKey=room_environment::roomKey(state.roomSeed,state.roomIndex)^static_cast<unsigned int>((i+1)*0x9e3779b9u);
            const auto facade=city_facade::grammar(c.depth,c.height,facadeKey);
            const VisualColor frame=facade.family==0?VisualColor{0.24f,0.25f,0.24f}:(facade.family==1?VisualColor{0.29f,0.25f,0.21f}:VisualColor{0.17f,0.20f,0.21f});
            const VisualColor glass=facade.family==1?VisualColor{0.055f,0.075f,0.082f}:VisualColor{0.026f,0.046f,0.058f};
            const float facadeZ=z0+c.center.z;
            const int authoredCadence=state.localSettings.graphicsPreset>=2?2:3;
            const bool authoredFacade=state.localSettings.graphicsPreset>0&&(i%authoredCadence)==0&&cityWindowBayModelList_&&cityWindowBaySmallModelList_&&cityDoorBayModelList_&&cityDoorWindowBayModelList_&&cityCorniceModelList_;
            const Quat facadeOrientation=quaternionFromEulerXYZ(0.0f,inward>0.0f?-PI*0.5f:PI*0.5f,0.0f);
            const auto atmosphereInputs=render_contract::effectiveAtmosphereInputs(lightingControl_,{state.time,state.roomIndex,state.vacuum.power*0.62f});
            const float nightPresence=0.34f+(1.0f-render_contract::celestialDaylight(atmosphereInputs.time))*0.66f;
            // Base, corner piers, cornice and crown establish a readable
            // construction hierarchy before any windows are added.
            drawBox({faceX,c.bottomY+facade.baseHeight*0.5f,facadeZ},{0.055f,facade.baseHeight,c.depth*1.015f},0,0,0,frame.r*0.72f,frame.g*0.72f,frame.b*0.72f);
            for(float edge:{-1.0f,1.0f})drawBox({faceX,c.center.y,facadeZ+edge*c.depth*0.48f},{0.065f,c.height*0.94f,0.10f},0,0,0,frame.r,frame.g,frame.b);
            for(int story=0;story<facade.stories;++story){
                const bool authoredStory=authoredFacade&&story<(state.localSettings.graphicsPreset>=2?3:2);
                const float y=city_facade::storyCenter(facade,story,c.bottomY);
                const float courseY=c.bottomY+static_cast<float>(story+1)*facade.storyHeight-facade.corniceHeight*0.5f;
                if(!authoredStory)drawBox({faceX,courseY,facadeZ},{0.070f,facade.corniceHeight,c.depth*0.985f},0,0,0,frame.r,frame.g,frame.b);
                for(int bay=0;bay<facade.bays;++bay){
                    const float z=city_facade::bayCenter(facade,bay,facadeZ);
                    const bool entrance=story==0&&bay==facade.entranceBay;
                    const float openingW=facade.bayWidth*(entrance?0.58f:facade.windowWidthRatio);
                    const float openingH=entrance?facade.storyHeight*0.70f:facade.storyHeight*facade.windowHeightRatio;
                    const float openingY=entrance?c.bottomY+openingH*0.5f+facade.baseHeight*0.18f:y+facade.storyHeight*0.04f;
                    drawBox({faceX+inward*facade.recessDepth,openingY,z},{0.034f,openingH,openingW},0,0,0,glass.r,glass.g,glass.b);
                    if(authoredStory){
                        const unsigned int module=entrance?(facade.family==2?cityDoorWindowBayModelList_:cityDoorBayModelList_):
                            (facade.family==2?cityWindowBaySmallModelList_:cityWindowBayModelList_);
                        // These are the kit's actual CC0 window and door
                        // assemblies, including separate frame and glass
                        // batches. We fit their one-bay proportions to the
                        // existing grammar while collision remains the mass.
                        drawStaticModel(module,{faceX+inward*0.052f,c.bottomY+story*facade.storyHeight,z},{facade.bayWidth,facade.storyHeight,0.12f},facadeOrientation);
                        gradedColor(frame.r*1.08f,frame.g*1.06f,frame.b*1.02f);
                        drawStaticModel(cityCorniceModelList_,{faceX+inward*0.065f,courseY-facade.corniceHeight*0.5f,z},{facade.bayWidth/15.0f,facade.corniceHeight,0.28f},facadeOrientation);
                    }else{
                        const float jamb=std::max(0.025f,facade.bayWidth*0.035f);
                        for(float edge:{-1.0f,1.0f})drawBox({faceX+inward*0.012f,openingY,z+edge*openingW*0.53f},{0.055f,openingH*1.08f,jamb},0,0,0,frame.r,frame.g,frame.b);
                        drawBox({faceX+inward*0.012f,openingY+openingH*0.54f,z},{0.055f,jamb,openingW*1.10f},0,0,0,frame.r,frame.g,frame.b);
                        drawBox({faceX+inward*0.020f,openingY-openingH*0.55f,z},{0.075f,jamb*1.25f,openingW*1.16f},0,0,0,frame.r*1.14f,frame.g*1.12f,frame.b*1.08f);
                        if(!entrance){
                            drawBox({faceX+inward*(facade.recessDepth+0.006f),openingY,z},{0.018f,openingH*0.035f,openingW*0.94f},0,0,0,frame.r*0.82f,frame.g*0.86f,frame.b*0.88f);
                            drawBox({faceX+inward*(facade.recessDepth+0.006f),openingY,z},{0.018f,openingH*0.94f,openingW*0.025f},0,0,0,frame.r*0.82f,frame.g*0.86f,frame.b*0.88f);
                        }
                    }
                    if(!entrance&&city_facade::windowOccupied(facadeKey,story,bay)){
                        const float warmth=city_facade::occupiedWindowWarmth(facadeKey,story,bay);
                        const VisualColor occupied{0.38f+warmth*0.12f,0.25f+warmth*0.08f,0.12f+warmth*0.035f};
                        glDisable(GL_LIGHTING);
                        // Sit just behind the visible frame but in front of the
                        // imported glass batch, whose authored depth otherwise
                        // masks the interior plane completely.
                        drawBox({faceX+inward*0.078f,openingY,z},{0.012f,openingH*0.66f,openingW*0.68f},0,0,0,occupied.r*nightPresence,occupied.g*nightPresence,occupied.b*nightPresence);
                        glEnable(GL_LIGHTING);
                    }
                    if(entrance)drawBox({faceX+inward*0.07f,c.bottomY+0.045f,z},{0.18f,0.09f,openingW*1.35f},0,0,0,0.18f,0.18f,0.17f);
                }
            }
            drawBox({c.center.x,c.topY+facade.crownHeight*0.5f,z0+c.center.z},{c.width*1.035f,facade.crownHeight,c.depth*1.035f},0,0,0,frame.r*1.08f,frame.g*1.06f,frame.b*1.02f);
            if(c.height>1.8f){drawBox({c.center.x-inward*c.width*0.20f,c.topY+facade.crownHeight+0.13f,z0+c.center.z+c.depth*0.18f},{c.width*0.24f,0.26f,c.depth*0.22f},0,0,0,0.23f,0.25f,0.25f);drawBox({c.center.x+inward*c.width*0.18f,c.topY+facade.crownHeight+0.10f,z0+c.center.z-c.depth*0.16f},{0.08f,0.20f,0.08f},0,0,0,0.16f,0.18f,0.18f);}
        }else if(field||coastal){
            const VisualColor growth=field?VisualColor{0.16f,0.34f,0.19f}:VisualColor{0.46f,0.48f,0.36f};
            drawBox({c.center.x,0.055f,z0+c.center.z-c.depth*0.46f},{c.width*0.76f,0.065f,0.035f},0,0,0,growth.r,growth.g,growth.b);
        }
        if(plan.setting==room_environment::RoomSetting::City&&plan.form==room_environment::RoomForm::Corridor&&room_environment::obstacleRole(plan,state.roomSeed,state.roomIndex,i)==room_environment::EnvironmentRole::Landmark){
            const float tierH=gameplay::WORLD_SCALE.storyHeight*0.34f;
            drawBox({c.center.x,c.topY+tierH*0.5f,z0+c.center.z},{c.width*0.58f,tierH,c.depth*0.62f},0,0,0,0.34f,0.40f,0.44f);
        }
    }
    for(int i=0;i<state.slopeSupportCount;++i)drawSlopeWedge(state.slopeSupports[i],z0,{0.39f,0.42f,0.36f});
    if(state.slopeLab)for(int i=0;i<state.rockSupportCount;++i){const auto& rock=state.rockSupports[i];const VisualColor substrate=roomSubstrateColor(plan.setting);drawFacetedRock(rock.prop,rock.roomSeed,rock.roomIndex,rock.propIndex,z0,{substrate.r*0.82f,substrate.g*0.82f,substrate.b*0.82f},stoneTexture_,1.35f);}
    const auto traversalPresentation=room_environment::traversalPresentationFor(plan.setting,state.roomInspector||state.traversalLab);
    const auto geometry=state.slopeLab?room_environment::RoomGeometryCapacityPlan{}:room_environment::roomGeometryCapacityPlan(plan,state.roomSeed,state.roomIndex,ROOM_COLLIDER_COUNT);
    for(int i=0;i<plan.traversal.surfaceCount;++i){const auto& surface=plan.traversal.surfaces[i];if(!geometry.traversalIncluded[i]||room_environment::usesShallowElevation(plan,surface))continue;const auto spec=room_environment::physicalTraversalObstacle(surface);
        drawBox(spec.center+Vec3{0,0,z0},spec.size,0,0,0,traversalPresentation.color.x,traversalPresentation.color.y,traversalPresentation.color.z);
    }
    if(plan.sidewalks&&!city){
        const bool canyon=plan.form==room_environment::RoomForm::Canyon,skyline=plan.form==room_environment::RoomForm::Skyline;
        const float walkX=canyon?3.55f:(skyline?8.15f:5.2f),walkW=canyon?1.1f:(skyline?2.4f:1.35f);
        drawBox({-walkX,0.025f,z0},{walkW,0.05f,ROOM_DEPTH-1.0f},0,0,0,0.43f,0.45f,0.46f);drawBox({walkX,0.025f,z0},{walkW,0.05f,ROOM_DEPTH-1.0f},0,0,0,0.43f,0.45f,0.46f);
    }
    for(int i=0;i<room_environment::environmentPropCount(plan);++i){
        if(!geometry.propIncluded[i])continue;
        const auto prop=room_environment::environmentProp(plan,state.roomSeed,state.roomIndex,i);const Vec3 p=prop.center+Vec3{0,0,z0};
        using room_environment::EnvironmentPrimitive;
        if(prop.primitive==EnvironmentPrimitive::House){
            int partIndex=0;for(const auto& part:house_geometry::parts(prop,z0)){const float variation=0.92f+0.045f*static_cast<float>((partIndex++ + prop.variant)%3);const VisualColor color=part.surface==0?VisualColor{0.40f*variation,0.47f*variation,0.50f*variation}:(part.surface==3?VisualColor{0.035f,0.045f,0.048f}:VisualColor{(part.surface==1?0.30f:0.26f)*variation,(part.surface==1?0.37f:0.32f)*variation,(part.surface==1?0.41f:0.36f)*variation});if(part.surface==0)drawTexturedBox(plan.setting==room_environment::RoomSetting::City?metalTexture_:stoneTexture_,part.center,part.size,0,part.yaw,0,color,1.35f,std::fmod((prop.variant+partIndex)*0.173f,1.0f));else drawBox(part.center,part.size,0,part.yaw,0,color.r,color.g,color.b);}
            const Vec3 front{std::sin(prop.yaw),0,std::cos(prop.yaw)},right{std::cos(prop.yaw),0,-std::sin(prop.yaw)};
            const Vec3 face=p+front*(prop.size.z*0.505f);
            const bool authoredCityLandmark=plan.setting==room_environment::RoomSetting::City&&prop.role==room_environment::EnvironmentRole::Landmark&&state.localSettings.graphicsPreset>0&&cityWindowBaySmallModelList_&&cityDoorBayModelList_;
            drawBox({p.x,0.028f,p.z},{prop.size.x*0.88f,0.055f,prop.size.z*0.88f},0,prop.yaw,0,0.07f,0.10f,0.10f);
            drawBox(p+Vec3{0,prop.size.y*0.79f,0},{prop.size.x*1.06f,0.075f,prop.size.z*1.06f},0,prop.yaw,0,0.18f,0.22f,0.23f);
            for(float side:{-1.0f,1.0f}){
                const Vec3 window=face+right*(side*prop.size.x*0.255f)+Vec3{0,prop.size.y*0.47f,0}+front*0.012f;
                if(authoredCityLandmark)drawStaticModel(cityWindowBaySmallModelList_,window-Vec3{0,prop.size.y*0.12f,0}+front*0.022f,{prop.size.x*0.25f,prop.size.y*0.24f,0.10f},quaternionFromEulerXYZ(0,prop.yaw,0));
                else{
                    drawBox(window,{prop.size.x*0.23f,prop.size.y*0.20f,0.028f},0,prop.yaw,0,0.025f,0.035f,0.040f);
                    drawBox(window+Vec3{0,prop.size.y*0.105f,0},{prop.size.x*0.27f,0.025f,0.045f},0,prop.yaw,0,0.42f,0.48f,0.47f);
                    drawBox(window-Vec3{0,prop.size.y*0.105f,0},{prop.size.x*0.27f,0.025f,0.045f},0,prop.yaw,0,0.28f,0.32f,0.32f);
                    drawBox(window,{0.024f,prop.size.y*0.23f,0.045f},0,prop.yaw,0,0.34f,0.39f,0.39f);
                }
            }
            const Vec3 step=face+Vec3{0,0.055f,0}+front*0.24f;
            drawBox(step,{prop.size.x*0.34f,0.11f,0.46f},0,prop.yaw,0,0.20f,0.22f,0.21f);
            if(authoredCityLandmark)drawStaticModel(cityDoorBayModelList_,face+front*0.030f,{prop.size.x*0.28f,prop.size.y*0.54f,0.12f},quaternionFromEulerXYZ(0,prop.yaw,0));
        }
        else if(prop.primitive==EnvironmentPrimitive::Tree){
            const unsigned int treeSeed=room_environment::roomKey(state.roomSeed,state.roomIndex)^static_cast<unsigned int>((i+1)*0x45d9f3bu);
            const auto treeProfile=tree_geometry::profile(prop);
            const auto authoredVariant=tree_geometry::authoredVariant(prop,treeSeed);
            const unsigned int authoredTree=treeProfile.archetype==tree_geometry::Archetype::Broad?commonTreeModelList_:
                (treeProfile.archetype==tree_geometry::Archetype::WindShaped?twistedTreeModelList_:pineTreeModelList_);
            if(authoredTree){
                const float widthCorrection=treeProfile.archetype==tree_geometry::Archetype::Broad?0.94f:
                    (treeProfile.archetype==tree_geometry::Archetype::WindShaped?0.82f:0.77f);
                // The converted models are one unit tall and grounded at Y=0.
                // Preserve their authored proportions, using prop height as the
                // single source of truth for climbing and world scale.
                glDisable(GL_LIGHTING);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
                drawGroundShadow(p,prop.size.x*0.18f,prop.size.z*0.16f,prop.size.y*0.32f,0.22f,0.0f);
                glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_LIGHTING);
                drawStaticModel(authoredTree,p+Vec3{0,0.008f,0},{prop.size.y*widthCorrection*authoredVariant.widthScale,prop.size.y,prop.size.y*widthCorrection*authoredVariant.depthScale},quaternionFromEulerXYZ(authoredVariant.pitch,prop.yaw+authoredVariant.yawJitter,authoredVariant.roll));
                const VisualColor bark{0.28f,0.205f,0.13f};
                const float rootRadius=std::max(prop.size.x,prop.size.z)*0.075f;
                for(int root=0;root<3;++root){
                    const float angle=prop.yaw+static_cast<float>(root)*PI*2.0f/3.0f+room_environment::unit(treeSeed+root)*0.22f;
                    const Vec3 rootEnd=p+Vec3{std::cos(angle)*rootRadius*3.2f*authoredVariant.rootSpread,0.018f,std::sin(angle)*rootRadius*3.2f*authoredVariant.rootSpread};
                    drawTaperedSegment(p+Vec3{0,rootRadius*0.30f,0},rootEnd,rootRadius*0.42f,0.018f,bark,6);
                }
            }else{
            const VisualColor bark=treeProfile.archetype==tree_geometry::Archetype::Broad?VisualColor{0.30f,0.235f,0.16f}:VisualColor{0.34f,0.27f,0.19f};
            const float trunkHeight=prop.size.y*treeProfile.trunkHeight,baseRadius=std::max(prop.size.x,prop.size.z)*treeProfile.baseRadius;
            const Vec3 lean{prop.size.x*treeProfile.leanX,0,prop.size.z*treeProfile.leanZ};
            const Vec3 trunkBase=p+Vec3{0,0.015f,0},trunkMid=p+Vec3{-prop.size.x*0.025f,trunkHeight*0.56f,prop.size.z*0.018f}+lean*0.38f,trunkTop=p+Vec3{0,trunkHeight,0}+lean;
            drawTaperedSegment(trunkBase,trunkMid,baseRadius,baseRadius*0.66f,bark,9);
            drawTaperedSegment(trunkMid,trunkTop,baseRadius*0.68f,baseRadius*0.34f,{0.38f,0.30f,0.21f},8);
            const int rootCount=treeProfile.archetype==tree_geometry::Archetype::Broad?7:5;
            for(int root=0;root<rootCount;++root){const float angle=prop.yaw+static_cast<float>(root)*PI*2.0f/static_cast<float>(rootCount)+room_environment::unit(treeSeed+root)*0.28f;const float rootReach=baseRadius*(treeProfile.archetype==tree_geometry::Archetype::Broad?4.2f:3.2f);const Vec3 rootEnd=p+Vec3{std::cos(angle)*rootReach,0.025f,std::sin(angle)*rootReach};drawTaperedSegment(p+Vec3{0,baseRadius*0.34f,0},rootEnd,baseRadius*0.38f,0.025f,bark,6);}
            for(int branch=0;branch<treeProfile.branchCount;++branch){
                float angle=prop.yaw+static_cast<float>(branch)*PI*2.0f/static_cast<float>(treeProfile.branchCount)+room_environment::unit(treeSeed+31u+branch)*0.46f;
                if(treeProfile.archetype==tree_geometry::Archetype::WindShaped)angle=prop.yaw+(branch&1?0.42f:-0.34f);
                const float branchY=trunkHeight*(0.48f+branch*0.055f),reach=prop.size.x*(0.25f+0.035f*static_cast<float>(branch%3));
                const Vec3 branchStart=p+Vec3{0,branchY,0}+lean*(branchY/trunkHeight);
                const Vec3 branchEnd=branchStart+Vec3{std::cos(angle)*reach,prop.size.y*(0.09f+0.018f*(branch&1)),std::sin(angle)*reach};
                drawTaperedSegment(branchStart,branchEnd,baseRadius*0.34f,baseRadius*0.085f,{0.36f,0.29f,0.20f},7);
            }
            bool activeClimb=false;
            if(state.player.treeClimbing&&tileIndex==state.topology.currentTileIndex&&state.player.treeCollider>=0&&state.player.treeCollider<state.debug.colliderCount){
                const RoomCollider& climbTree=state.roomColliders[state.player.treeCollider];
                activeClimb=climbTree.kind==RoomColliderKind::TreeTrunk&&std::abs(climbTree.center.x-prop.center.x)<0.05f&&std::abs(climbTree.center.z-prop.center.z)<0.05f;
            }
            int crownIndex=0;for(const auto& part:tree_geometry::crownParts(prop,z0)){
                const float normalizedHeight=(part.center.y-p.y)/std::max(0.001f,prop.size.y);
                Vec3 crownCenter=part.center+lean*std::min(1.0f,normalizedHeight);
                if(treeProfile.archetype==tree_geometry::Archetype::WindShaped)crownCenter+=Vec3{prop.size.x*0.10f*(0.45f+crownIndex*0.08f),0,-prop.size.z*0.035f};
                const Vec3 crownSize{part.size.x*treeProfile.crownWidth,part.size.y*treeProfile.crownHeight,part.size.z*treeProfile.crownWidth};
                const float radius=std::max(crownSize.x,crownSize.z)*0.58f;
                const bool occluding=activeClimb&&room_environment::treeFoliageClusterOccludes(state.camera.pos,state.player.pos,crownCenter,radius);
                if(occluding){glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);}
                const float variation=0.88f+0.045f*static_cast<float>((crownIndex+prop.variant)%4);
                // The CC0 atlas already carries natural hue and value.  Tint
                // only supplies family/season drift; multiplying it by the
                // old synthetic foliage color crushed the cards to black.
                const VisualColor foliage=treeProfile.archetype==tree_geometry::Archetype::Sentinel?VisualColor{0.72f*variation,0.86f*variation,0.74f*variation}:(treeProfile.archetype==tree_geometry::Archetype::Broad?VisualColor{0.82f*variation,0.92f*variation,0.72f*variation}:VisualColor{0.88f*variation,0.88f*variation,0.66f*variation});
                drawFoliageCards(crownCenter,crownSize,part.yaw,foliage,treeSeed+static_cast<unsigned int>(crownIndex++)*53u);
                if(occluding){glDepthMask(GL_TRUE);glDisable(GL_BLEND);}
            }
            }
        }
        else if(prop.primitive==EnvironmentPrimitive::LawnFragment)drawTexturedBox(fieldGrassTexture_,p,prop.size,0,prop.yaw,0,{0.72f,0.84f,0.70f},1.45f,std::fmod(prop.variant*0.219f,1.0f));
        else if(prop.primitive==EnvironmentPrimitive::Ruin){int ruinIndex=0;for(const auto& part:ruin_geometry::parts(prop,z0)){const float phase=std::fmod((prop.variant+ruinIndex)*0.137f,1.0f);const float weather=0.88f+0.055f*static_cast<float>((ruinIndex++ + prop.variant)%3);const VisualColor color=part.surface==0?VisualColor{0.84f*weather,0.82f*weather,0.72f*weather}:VisualColor{0.70f*weather,0.70f*weather,0.66f*weather};drawTexturedBox(stoneTexture_,part.center,part.size,0,0,0,color,1.25f,phase);if(part.size.y>0.42f)drawBox({part.center.x,part.center.y-part.size.y*0.39f,part.center.z-part.size.z*0.505f},{part.size.x*0.74f,0.045f,0.024f},0,0,0,0.20f,0.27f,0.20f);}}
        else if(prop.primitive==EnvironmentPrimitive::Rock){const VisualColor substrate=roomSubstrateColor(plan.setting);drawFacetedRock(prop,state.roomSeed,state.roomIndex,i,z0,{substrate.r*0.82f,substrate.g*0.82f,substrate.b*0.82f},stoneTexture_,1.35f);}
        else {for(const auto& part:marker_pillar_geometry::parts(prop,z0)){const VisualColor color=part.surface==0?VisualColor{0.70f,0.76f,0.78f}:VisualColor{0.72f,0.90f,0.94f};if(part.surface==0)drawTexturedBox(metalTexture_,part.center,part.size,0,0,0,color,0.92f);else drawBox(part.center,part.size,0,0,0,color.r,color.g,color.b);}}
    }
    // Bird sprites are withheld until their atlas rows, schedules, and world
    // placement form a complete behavior instead of intermittent billboards.
    if(plan.grass){const int maximum=state.localSettings.graphicsPreset<=0?room_environment::GrassBladeCountLow:room_environment::GrassBladeCountHigh,grassCount=static_cast<int>(maximum*plan.grassAmount);room_environment::GrassReactionInputs reaction{state.player.pos,state.phoneTransform.vacuumPullPoint,state.environmentVisual.latestShotOrigin,state.vacuum.power,state.environmentVisual.latestShotAge};const VisualColor bladeRoot=gummyMutation?VisualColor{0.72f,0.12f,0.46f}:(festiveMutation?VisualColor{0.56f,0.68f,0.68f}:VisualIdentity::GrassRoot);const VisualColor bladeTip=gummyMutation?VisualColor{0.98f,0.48f,0.82f}:(festiveMutation?VisualColor{0.90f,0.97f,1.0f}:VisualIdentity::GrassTip);glBegin(GL_TRIANGLES);for(int i=0;i<grassCount;++i){auto blade=room_environment::grassBlade(state.roomSeed,state.roomIndex,tileIndex,i);blade.root.z+=z0;const Vec3 tip=room_environment::grassTip(blade,state.time,reaction),side{std::cos(blade.phase)*blade.width*0.5f,0,std::sin(blade.phase)*blade.width*0.5f};const Vec3 rootL=blade.root-side,rootR=blade.root+side;const float tone=0.82f+room_environment::unit(static_cast<unsigned int>(i*17+tileIndex*131))*0.22f;gradedColor(bladeRoot.r*tone,bladeRoot.g*tone,bladeRoot.b*tone);glNormal3f(0.18f*std::cos(blade.phase),0.96f,0.18f*std::sin(blade.phase));glVertex3f(rootL.x,rootL.y,rootL.z);glVertex3f(rootR.x,rootR.y,rootR.z);gradedColor(bladeTip.r*tone,bladeTip.g*tone,bladeTip.b*tone);glVertex3f(tip.x,tip.y,tip.z);}glEnd();}
}

void DesktopRenderer::applyCamera(const GameState& state, float aspect) {
    Vec3 renderPos=state.camera.pos,renderTarget=state.camera.lookTarget;
    float renderFov=state.camera.verticalFovDegrees;
    const bool gameplayCamera=state.started&&!state.camera.firstPerson&&state.camera.spectatedPlayerId<0&&!state.cinematic.introActive&&!state.cinematic.deathActive;
    if(gameplayCamera){
        float capture=0.0f;Vec3 soulFocus=renderTarget;
        for(const auto& target:state.targets)if(target.alive&&target.slurpable){
            const float activity=clampf(std::max({target.vacuumPullAmount,target.soulVisual.latchAmount*0.72f,target.ingestProgress}),0.0f,1.0f);
            if(activity>capture){capture=activity;soulFocus=target.pos+Vec3{0,0.57f+target.soulVisual.verticalOffset,0};}
        }
        const float captureEase=capture*capture*(3.0f-2.0f*capture);
        const Vec3 forward=normalized(renderTarget-renderPos),right=normalized(cross3(forward,{0,1,0}));
        // Extraction gains weight through a slow focus pull and restrained
        // dolly. The world-space camera state remains authoritative for aim.
        renderTarget+=(soulFocus-renderTarget)*(captureEase*0.085f);
        renderPos-=forward*(captureEase*0.105f);
        renderPos.y-=captureEase*0.018f;
        renderFov+=captureEase*1.15f;
        // Critical impact is a brief directional recoil rather than broadband
        // shake; amplitude is deliberately below normal camera follow error.
        const float impact=clampf(state.hud.criticalHitPulse,0.0f,1.0f);
        const float recoil=impact*impact;
        renderPos-=forward*(recoil*0.045f);
        const float lateral=std::sin(state.time*31.0f)*recoil*0.014f;
        renderPos+=right*lateral;renderTarget+=right*(lateral*0.42f);
        renderFov-=recoil*0.65f;
    }
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); perspective(renderFov, aspect, VisualIdentity::CameraNearPlane, VisualIdentity::CameraFarPlane);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity(); lookAt(renderPos, renderTarget, {0,1,0});
}

void DesktopRenderer::drawHud(const GameState& state) const {
    // GLFW reports the Retina backing framebuffer here, not macOS logical
    // points. Render HUD geometry on a bounded logical canvas so a 2x backing
    // scale does not make every label and meter appear half-sized. Keeping the
    // aspect ratio intact also makes the same rule useful at 1440p and 4K.
    const int framebufferWidth=width_,framebufferHeight=height_;
    const float hudScale=clampf(std::min(static_cast<float>(framebufferWidth)/1280.0f,static_cast<float>(framebufferHeight)/720.0f),1.0f,2.5f);
    struct RestoreFramebufferSize { int& width;int& height;int oldWidth;int oldHeight;~RestoreFramebufferSize(){width=oldWidth;height=oldHeight;} } restore{width_,height_,framebufferWidth,framebufferHeight};
    width_=std::max(1,static_cast<int>(std::lround(framebufferWidth/hudScale)));
    height_=std::max(1,static_cast<int>(std::lround(framebufferHeight/hudScale)));
    float overlayAlpha=1.0f;
    const auto quad=[&](float x,float y,float w,float h,float r,float g,float b,float a) {
        glColor4f(r,g,b,a*overlayAlpha);
        glBegin(GL_QUADS);
        glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h);
        glEnd();
    };
    const auto rotatedQuad=[&](float cx,float cy,float w,float h,float angle,float r,float g,float b,float a) {
        const float c=std::cos(angle),s=std::sin(angle),hx=w*0.5f,hy=h*0.5f;
        const Vec3 corners[4]={{-hx,-hy,0},{hx,-hy,0},{hx,hy,0},{-hx,hy,0}};
        glColor4f(r,g,b,a*overlayAlpha); glBegin(GL_QUADS);
        for(const Vec3& p:corners) glVertex2f(cx+p.x*c-p.y*s,cy+p.x*s+p.y*c);
        glEnd();
    };
    const auto text=[&](const std::string& value,float x,float y,float scale,float r=1.0f,float g=1.0f,float b=1.0f,float a=0.94f){
        float pen=x;for(char c:value){if(c==' '){pen+=6*scale;continue;}const auto rows=bitmapGlyph(c);for(int row=0;row<7;++row)for(int col=0;col<5;++col)if(rows[row]&(1u<<(4-col))){const float px=pen+col*scale,py=y+row*scale;if(overlayAlpha<0.999f){quad(px-1,py,scale,scale,0,0,0,a);quad(px+1,py,scale,scale,0,0,0,a);quad(px,py-1,scale,scale,0,0,0,a);quad(px,py+1,scale,scale,0,0,0,a);}quad(px,py,scale,scale,r,g,b,a);}pen+=6*scale;}
    };
    const auto rainbow=[&](float hue){hue-=std::floor(hue);const float x=hue*6.0f,i=std::floor(x),f=x-i,q=1.0f-f;switch(static_cast<int>(i)%6){case 0:return Vec3{1,f,0};case 1:return Vec3{q,1,0};case 2:return Vec3{0,1,f};case 3:return Vec3{0,q,1};case 4:return Vec3{f,0,1};default:return Vec3{1,0,q};}};

    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0.0,static_cast<double>(width_),static_cast<double>(height_),0.0,-1.0,1.0);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    const float menuUiScale=clampf(std::min(static_cast<float>(width_)/1280.0f,static_cast<float>(height_)/720.0f),0.55f,1.8f);
    const float menuCanvasW=static_cast<float>(width_)/menuUiScale,menuCanvasH=static_cast<float>(height_)/menuUiScale;
    if(state.localSettings.fpsCounter){const std::string fps="FPS "+std::to_string(static_cast<int>(std::round(displayedFps)));text(fps,width_-fps.size()*7.2f-12,68,1.2f,0.72f,1.0f,0.90f);}
    if(state.attractMode){
        const float cx=width_*0.5f;
        const float exitLinear=state.cinematic.attractExitActive?clampf(state.cinematic.attractExitElapsed/0.62f,0.0f,1.0f):0.0f;
        const float exitEase=exitLinear*exitLinear*(3.0f-2.0f*exitLinear);
        quad(0,0,static_cast<float>(width_),static_cast<float>(height_),0.0f,0.0f,0.0f,0.10f+exitEase*0.90f);
        const float titleOpacity=1.0f-clampf((exitEase-0.68f)/0.32f,0.0f,1.0f);
        drawPaletteMenuTitle("DATA",cx,height_*(0.19f+exitEase*0.25f),(96.0f-exitEase*26.0f)*menuUiScale,state.time,titleOpacity);
        glMatrixMode(GL_MODELVIEW);glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);glDisable(GL_BLEND);glEnable(GL_DEPTH_TEST);glEnable(GL_LIGHTING);return;
    }
    const bool pausedSolo=state.started&&state.uiPaused&&!state.multiplayer.enabled&&!state.upgradeMenu.active;
    if(state.dead){
        const float fade=clampf(state.cinematic.overlayFade,0.0f,1.0f);
        quad(0,0,static_cast<float>(width_),static_cast<float>(height_),0.0f,0.0f,0.0f,0.18f*fade);
        const float awaken=clampf(state.cinematic.restartAwaken,0.0f,1.0f);
        const float pulse=0.5f+0.5f*std::sin(state.time*1.5f);
        const float titleScale=2.6f+awaken*0.35f;
        const std::string again="Again?";
        const std::string quit="Quit";
        const float cx=width_*0.5f;
        const float cy=height_*0.56f;
        const auto drawDeathChoice=[&](const std::string& label,int choice,float y){
            const bool selected=state.cinematic.deathChoice==choice;
            const float scale=choice==0?titleScale:1.55f;
            const float tw=static_cast<float>(label.size())*6.0f*scale;
            const float alpha=(selected?0.98f:0.54f)*fade;
            if(selected){
                quad(cx-tw*0.5f-24.0f,y+3.5f*scale,7.0f,7.0f,VisualIdentity::ElectricCyan.r,VisualIdentity::ElectricCyan.g,VisualIdentity::ElectricCyan.b,(0.78f+0.16f*pulse)*fade);
            }
            text(label,cx-tw*0.5f,y,scale,selected?1.0f:0.70f,selected?1.0f:0.88f,1.0f,alpha);
        };
        drawDeathChoice(again,0,cy);
        drawDeathChoice(quit,1,cy+58.0f);
        glMatrixMode(GL_MODELVIEW);glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);glDisable(GL_BLEND);glEnable(GL_DEPTH_TEST);glEnable(GL_LIGHTING);return;
    }
    if(state.cinematic.introActive||!state.started||pausedSolo){
        if(state.cinematic.menuEnterActive){
            const float linear=clampf(state.cinematic.menuEnterElapsed/0.48f,0.0f,1.0f);
            const float fade=1.0f-linear*linear*(3.0f-2.0f*linear);
            quad(0,0,static_cast<float>(width_),static_cast<float>(height_),0.0f,0.0f,0.0f,fade);
        }
        glMatrixMode(GL_MODELVIEW);glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);glDisable(GL_BLEND);glEnable(GL_DEPTH_TEST);glEnable(GL_LIGHTING);return;
    }

    if(state.multiplayer.enabled){const std::string code=state.multiplayer.roomCode.data();const std::string net=state.multiplayer.status.data(),focus=code.empty()?net:code;const float w=std::max(92.0f,static_cast<float>(focus.size())*8.1f+18.0f),x=width_-w-12.0f;quad(x,12,w,30,0.005f,0.012f,0.016f,0.54f);text(focus,x+(w-focus.size()*7.2f)*0.5f,20,1.2f,0.66f,0.96f,1.0f);}
    if(state.camera.spectatedPlayerId>=0){
        const std::string label="SPECTATING  P"+std::to_string(state.camera.spectatedPlayerId+1);
        const float scale=1.35f,tw=label.size()*6.0f*scale,pw=tw+24.0f,px=(width_-pw)*0.5f;
        quad(px,18,pw,24,0.005f,0.012f,0.016f,0.62f);
        quad(px,18,pw,1.5f,VisualIdentity::ElectricCyan.r,VisualIdentity::ElectricCyan.g,VisualIdentity::ElectricCyan.b,0.82f);
        text(label,px+12.0f,25,scale,0.72f,0.96f,1.0f,0.94f);
        glMatrixMode(GL_MODELVIEW);glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);glDisable(GL_BLEND);glEnable(GL_DEPTH_TEST);glEnable(GL_LIGHTING);return;
    }

    if(state.traversalLab){
        const float panelW=520.0f,panelX=(width_-panelW)*0.5f;
        quad(panelX,12,panelW,74,0.005f,0.012f,0.016f,0.72f);
        quad(panelX,12,panelW,1,VisualIdentity::ElectricCyan.r,VisualIdentity::ElectricCyan.g,VisualIdentity::ElectricCyan.b,0.78f);
        text("TRAVERSAL LAB",panelX+12,20,1.65f,0.82f,1.0f,0.94f);
        text("CENTER  GAPS 1.5  2.0  2.5",panelX+12,40,1.15f,1.0f,1.0f,1.0f);
        text("LEFT  LEDGES     RIGHT  ASCENT",panelX+12,55,1.05f,0.72f,0.94f,1.0f);
        text("SPACE JUMP   F LUNGE   SHIFT SPRINT",panelX+12,70,1.0f,0.82f,1.0f,0.88f);
    }
    if(state.roomInspector){
        const auto& r=state.roomInspectorReport;const float panelW=std::min(780.0f,static_cast<float>(width_)-24.0f),panelX=(width_-panelW)*0.5f;
        quad(panelX,12,panelW,148,0.005f,0.012f,0.016f,0.82f);quad(panelX,12,panelW,1,VisualIdentity::ElectricCyan.r,VisualIdentity::ElectricCyan.g,VisualIdentity::ElectricCyan.b,0.82f);
        text(std::string(room_environment::premiseName(r.premise))+"  "+room_environment::settingName(r.setting)+" / "+room_environment::formName(r.form)+" / "+room_environment::scaleName(r.scale)+" / "+room_environment::conditionName(r.condition)+" / INTENT "+room_environment::traversalIntentName(r.traversalIntent),panelX+12,20,1.30f,0.82f,1.0f,0.94f);
        text("SEED "+std::to_string(r.seed)+"  ROOM "+std::to_string(r.roomIndex)+"  ROUTE "+(r.requiredRouteValid?"VALID":"INVALID"),panelX+12,42,1.05f,1.0f,1.0f,1.0f);
        text("SURF "+std::to_string(r.traversalSurfaceCount)+"  EDGE "+std::to_string(r.traversalEdgeCount)+" (REQ "+std::to_string(r.requiredEdgeCount)+")  COLLIDER "+std::to_string(r.colliderCount)+"  PROP "+std::to_string(r.presentationPropCount),panelX+12,61,1.0f,0.72f,0.94f,1.0f);
        text("ROLE MASS "+std::to_string(r.environmentRoleCounts[static_cast<int>(room_environment::EnvironmentRole::Mass)])+"  LANDMARK "+std::to_string(r.environmentRoleCounts[static_cast<int>(room_environment::EnvironmentRole::Landmark)])+"  TRAVERSAL "+std::to_string(r.environmentRoleCounts[static_cast<int>(room_environment::EnvironmentRole::Traversal)])+"  DETAIL "+std::to_string(r.environmentRoleCounts[static_cast<int>(room_environment::EnvironmentRole::Detail)])+"  HUMAN "+std::to_string(gameplay::WORLD_SCALE.humanHeight).substr(0,4)+"  STORY "+std::to_string(gameplay::WORLD_SCALE.storyHeight).substr(0,4),panelX+12,79,0.88f,0.82f,0.94f,1.0f);
        text("ENEMY "+std::to_string(r.enemyCount)+" / "+std::to_string(r.enemyBudget)+"  TRANSPARENT "+std::to_string(r.transparentPrimitiveCount)+"  VISIBLE~ "+std::to_string(r.visiblePrimitiveEstimate)+"  DRAW "+(r.drawCallBucket==0?"LOW":(r.drawCallBucket==1?"MED":"HIGH")),panelX+12,96,1.0f,0.82f,1.0f,0.88f);
        text(std::string("[ ] PREMISE  R SEED  E ENEMIES ")+(state.roomInspectorEnemies?"ON":"OFF"),panelX+12,119,1.0f,1.0f,1.0f,1.0f);
        text("5 KEEP  6 TUNE  7 REDESIGN  8 REMOVE",panelX+12,133,1.0f,0.72f,1.0f,0.74f);
    }
    if((state.traversalLab||state.slopeLab||state.rallyLab||state.roomInspector)&&state.hud.buildLabel[0])
        text(state.hud.buildLabel.data(),12,336,1.0f,0.58f,0.92f,1.0f,0.82f);
    // The collision-authoritative head center owns an actionable data glyph.
    // Body motion leads; the symbol only reinforces combat timing or impact.
    overlayAlpha=state.hud.critMarkerOpacity;
    {const Vec3 viewForward=normalized(state.camera.lookTarget-state.camera.pos),viewRight=normalized(cross3(viewForward,{0,1,0})),viewUp=cross3(viewRight,viewForward);const float tanHalf=std::tan(state.camera.verticalFovDegrees*PI/360.0f),aspect=static_cast<float>(width_)/std::max(1,height_);constexpr char glyphs[]="01ABCDEFHIKMNPRSTXYZ+-/:";for(int i=0;i<TARGET_COUNT;++i){const TargetState& target=state.targets[i];if(!target.alive||target.slurpable)continue;const float attackT=target.attackTimer>0?1-clampf(target.attackTimer/HUMAN_SWING_ATTACK_DURATION,0,1):-1.0f;if(target.attackTimer<=0.0f&&target.hitFlash<=0.08f)continue;const float attackBob=target.attackTimer>0?std::sin(attackT*PI)*0.035f*(target.attackVariant>=2?1.0f:0.0f):0;const Vec3 world{target.pos.x,(HUMAN_VISUAL_SPEC.totalHeight-HUMAN_VISUAL_SPEC.headRadius)*target.scale+attackBob,target.pos.z},delta=world-state.camera.pos;const float depth=dot3(delta,viewForward);if(depth<=0.18f||depth>16.0f)continue;const float nx=dot3(delta,viewRight)/(depth*tanHalf*aspect),ny=dot3(delta,viewUp)/(depth*tanHalf);if(std::abs(nx)>1.04f||std::abs(ny)>1.04f)continue;const float armorMax=target.brute?4.0f:2.0f,damage=1.0f-clampf(target.armor/armorMax,0,1);const bool perfectReady=attackT>=0.22f&&attackT<=0.46f;const Vec3 baseColor=rainbow(0.51f+damage*0.38f);const Vec3 magenta{VisualIdentity::ElectricMagenta.r,VisualIdentity::ElectricMagenta.g,VisualIdentity::ElectricMagenta.b};const Vec3 color=mix3(baseColor,magenta,clampf(state.hud.criticalHitPulse*0.75f+(perfectReady?0.18f:0.0f),0.0f,1.0f));const int cycle=(static_cast<int>(state.time*10.0f)+i*7+state.roomIndex*3)%static_cast<int>(sizeof(glyphs)-1);const float perspectiveScale=clampf(8.0f/depth,0.82f,1.55f),marker=(11.0f+damage*4.0f+(perfectReady?3.0f:0))*perspectiveScale,scale=(1.35f+damage*0.28f+(perfectReady?0.18f:0))*perspectiveScale,sx=(nx*0.5f+0.5f)*width_,sy=(0.5f-ny*0.5f)*height_,alpha=0.72f+damage*0.20f+(perfectReady?0.08f:0),spin=state.time*0.9f+i*0.37f;rotatedQuad(sx,sy,marker,marker,PI*0.25f+spin,color.x,color.y,color.z,0.10f+damage*0.08f);rotatedQuad(sx-marker,sy,marker*0.52f,2,spin*0.08f,color.x,color.y,color.z,alpha);rotatedQuad(sx+marker,sy,marker*0.52f,2,spin*0.08f,color.x,color.y,color.z,alpha);rotatedQuad(sx,sy-marker,2,marker*0.52f,spin*0.08f,color.x,color.y,color.z,alpha);rotatedQuad(sx,sy+marker,2,marker*0.52f,spin*0.08f,color.x,color.y,color.z,alpha);text(std::string(1,glyphs[cycle]),sx-2.5f*scale+1,sy-3.5f*scale+1,scale,0,0,0,alpha*0.85f);text(std::string(1,glyphs[cycle]),sx-2.5f*scale,sy-3.5f*scale,scale,color.x,color.y,color.z,alpha);}}
    overlayAlpha=1.0f;
    {const auto labelFor=[](int signal)->const char*{switch(signal){case 1:return "HELP";case 2:return "PING";case 3:return "GROUP";case 4:return "OK";default:return "";}};const auto colorFor=[](int signal)->VisualColor{switch(signal){case 1:return VisualIdentity::ElectricMagenta;case 2:return VisualIdentity::ElectricCyan;case 3:return VisualIdentity::AcidChartreuse;case 4:return VisualIdentity::WarmGold;default:return VisualIdentity::ElectricCyan;}};const Vec3 viewForward=normalized(state.camera.lookTarget-state.camera.pos),viewRight=normalized(cross3(viewForward,{0,1,0})),viewUp=cross3(viewRight,viewForward);const float tanHalf=std::tan(state.camera.verticalFovDegrees*PI/360.0f),aspect=static_cast<float>(width_)/std::max(1,height_);const auto drawSignal=[&](const PlayerState& player){if(player.commSignal<1||player.commSignal>4||player.commSignalTimer<=0.0f)return;const Vec3 world=player.pos+Vec3{0,1.05f,0},delta=world-state.camera.pos;const float depth=dot3(delta,viewForward);if(depth<=0.18f||depth>24.0f)return;const float nx=dot3(delta,viewRight)/(depth*tanHalf*aspect),ny=dot3(delta,viewUp)/(depth*tanHalf);if(std::abs(nx)>1.08f||std::abs(ny)>1.08f)return;const char* label=labelFor(player.commSignal);const VisualColor c=colorFor(player.commSignal);const float sx=(nx*0.5f+0.5f)*width_,sy=(0.5f-ny*0.5f)*height_,fade=clampf(player.commSignalTimer/0.35f,0.0f,1.0f),scale=clampf(9.0f/depth,1.15f,2.15f),tw=std::strlen(label)*6.0f*scale,pw=tw+18.0f*scale,ph=13.0f*scale,pulse=0.5f+0.5f*std::sin(state.time*8.0f);quad(sx-pw*0.5f,sy-ph*0.5f,pw,ph,VisualIdentity::DeepPlum.r*0.12f,VisualIdentity::DeepPlum.g*0.12f,VisualIdentity::DeepPlum.b*0.12f,0.52f*fade);quad(sx-pw*0.5f,sy-ph*0.5f,pw,1.4f*scale,c.r,c.g,c.b,(0.58f+0.18f*pulse)*fade);text(label,sx-tw*0.5f,sy-3.5f*scale,scale,c.r,c.g,c.b,0.96f*fade);};drawSignal(state.player);for(const auto& peer:state.multiplayer.peers)if(peer.active)drawSignal(peer.player);}

    // Reflex-critical state remains glanceable without duplicating the phone's
    // detailed instrument panel. Objective text appears only after progress or
    // completion gives the player a reason to consult it.
    {const float charge=clampf(state.player.battery/100.0f,0.0f,1.0f);const bool low=state.hud.lowBattery;const VisualColor chargeColor=low?VisualIdentity::Copper:VisualIdentity::ElectricCyan;const float w=220.0f,h=5.0f,x=(width_-w)*0.5f,y=22.0f;quad(x,y,w,h,VisualIdentity::DeepPlum.r,VisualIdentity::DeepPlum.g,VisualIdentity::DeepPlum.b,0.54f);quad(x,y,std::max(h,w*charge),h,chargeColor.r,chargeColor.g,chargeColor.b,low?0.96f:0.68f);if(state.roomClear||state.depositedSouls>0){const std::string objective=state.roomClear?"PATH OPEN":std::to_string(state.depositedSouls)+" / "+std::to_string(std::max(1,state.requiredSouls));const float s=1.0f;text(objective,width_-22.0f-objective.size()*6.0f*s,height_-35.0f,s,state.roomClear?VisualIdentity::AcidChartreuse.r:VisualIdentity::ElectricCyan.r,state.roomClear?VisualIdentity::AcidChartreuse.g:VisualIdentity::ElectricCyan.g,state.roomClear?VisualIdentity::AcidChartreuse.b:VisualIdentity::ElectricCyan.b,0.74f);}}
    if(std::max(state.hud.headshotPulse,state.hud.criticalHitPulse)>0.001f){const float charge=clampf(state.hud.headshotKillCharge,0,1),pulse=std::max(state.hud.headshotPulse,state.hud.criticalHitPulse),eased=pulse*pulse,w=static_cast<float>(width_),h=static_cast<float>(height_),breath=0.5f+0.5f*std::sin(state.time*2.4f),mist=12.0f+charge*9.0f+state.hud.perfectPulse*4.0f;const Vec3 magenta{VisualIdentity::ElectricMagenta.r,VisualIdentity::ElectricMagenta.g,VisualIdentity::ElectricMagenta.b};const Vec3 violet{0.58f,0.34f,0.92f};const Vec3 cyan{VisualIdentity::ElectricCyan.r,VisualIdentity::ElectricCyan.g,VisualIdentity::ElectricCyan.b};const Vec3 core=mix3(violet,magenta,0.72f),accent=mix3(cyan,magenta,0.46f);const float veil=eased*(0.035f+charge*0.070f),wisp=pulse*(0.075f+charge*0.105f),spark=pulse*(0.10f+charge*0.14f);quad(0,0,w,mist,core.x,core.y,core.z,veil);quad(0,h-mist,w,mist,accent.x,accent.y,accent.z,veil);quad(0,0,mist,h,accent.x,accent.y,accent.z,veil*0.90f);quad(w-mist,0,mist,h,core.x,core.y,core.z,veil*0.90f);for(int i=0;i<3;++i){const float phase=state.time*(0.55f+i*0.17f)+i*2.1f,drift=0.5f+0.5f*std::sin(phase),len=w*(0.22f+0.10f*i+0.08f*breath),thick=1.2f+i*1.1f+charge*1.4f,alpha=wisp*(0.72f-0.14f*i),x=clampf(drift*(w+len)-len,0.0f,w-len);const Vec3 c=i==1?accent:core;quad(x,2.0f+i*4.0f,len,thick,c.x,c.y,c.z,alpha);quad(w-x-len,h-3.0f-i*4.4f,len,thick,c.x,c.y,c.z,alpha*0.82f);const float y=clampf((0.5f+0.5f*std::sin(phase*0.81f+1.7f))*(h+len)-len,0.0f,h-len);quad(2.0f+i*4.0f,y,thick,len,c.x,c.y,c.z,alpha*0.70f);quad(w-3.0f-i*4.4f,h-y-len,thick,len,c.x,c.y,c.z,alpha*0.64f);}const float corner=clampf(std::min(w,h)*0.10f,34.0f,84.0f);quad(0,0,corner,2.0f,core.x,core.y,core.z,spark);quad(0,0,2.0f,corner,accent.x,accent.y,accent.z,spark*0.85f);quad(w-corner,h-2.0f,corner,2.0f,accent.x,accent.y,accent.z,spark*0.75f);quad(w-2.0f,h-corner,2.0f,corner,core.x,core.y,core.z,spark*0.65f);}

    if(state.hud.energyTicker[0]&&state.time<state.hud.energyTickerUntil){const std::string ticker=state.hud.energyTicker.data();const float scale=1.35f,tw=ticker.size()*6*scale,pw=std::max(118.0f,tw+16.0f),px=(width_-pw)*0.5f;const int type=state.hud.energyTickerType;const VisualColor tickerColor=type==1?VisualIdentity::Copper:(type==0?VisualIdentity::SignalGreen:VisualIdentity::ElectricCyan);quad(px,72,pw,18,0,0,0,0.54f);quad(px,72,pw,1,tickerColor.r,tickerColor.g,tickerColor.b,0.72f);text(ticker,(width_-tw)*0.5f,77,scale,tickerColor.r,tickerColor.g,tickerColor.b);}
    if(state.player.grabbedByTarget>=0){const std::string hint="WIGGLE  A  D";const float s=1.7f;text(hint,(width_-hint.size()*6*s)*0.5f,height_*0.69f,s,1.0f,0.82f,0.68f,0.94f);}
    if(state.player.downed){const std::string hint="SIGNAL DOWN  "+std::to_string(static_cast<int>(std::ceil(state.player.bleedoutTimer)));const float s=1.8f;text(hint,(width_-hint.size()*6*s)*0.5f,height_*0.55f,s,1.0f,0.48f,0.42f,0.96f);}
    if(state.player.inSecretRoom){const std::string hint=state.secretTv.broken?"NO SIGNAL":"SIGNAL "+std::to_string(state.secretTv.signal)+"   SHOOT TO DONATE";const float s=1.35f;text(hint,(width_-hint.size()*6*s)*0.5f,54,s,0.72f,0.94f,0.96f,0.88f);}

    // One persistent reticle rotor owns four independently translated arms.
    const float cx=width_*0.5f, cy=height_*0.5f;
    const float spread=state.hud.crosshairSpreadPixels,arm=14.0f,thick=3.0f;
    const float angle=state.hud.crosshairRotationDegrees*PI/180.0f;
    const bool joining=state.hud.shootJoinTimer>0.0f;
    const float rr=joining?1.0f:0.498f,rg=joining?1.0f:0.906f,rb=1.0f;
    const float reticleAlpha=0.98f*clampf(state.hud.crosshairOpacity,0.0f,1.0f);
    const auto rotateCenter=[&](float x,float y){return Vec3{cx+x*std::cos(angle)-y*std::sin(angle),cy+x*std::sin(angle)+y*std::cos(angle),0};};
    Vec3 center=rotateCenter(0,-spread); rotatedQuad(center.x,center.y,thick,arm,angle,rr,rg,rb,reticleAlpha);
    center=rotateCenter(0,spread); rotatedQuad(center.x,center.y,thick,arm,angle,rr,rg,rb,reticleAlpha);
    center=rotateCenter(-spread,0); rotatedQuad(center.x,center.y,arm,thick,angle,rr,rg,rb,reticleAlpha);
    center=rotateCenter(spread,0); rotatedQuad(center.x,center.y,arm,thick,angle,rr,rg,rb,reticleAlpha);

    if(state.upgradeMenu.active){
        quad(0,0,static_cast<float>(width_),static_cast<float>(height_),0.0f,0.012f,0.018f,0.54f);
        glPushMatrix();glScalef(menuUiScale,menuUiScale,1.0f);
        const float pw=std::min(680.0f,menuCanvasW-24.0f),ph=300.0f,px=(menuCanvasW-pw)*0.5f,py=(menuCanvasH-ph)*0.5f;
        const float arrival=clampf(state.upgradeMenu.presentationTime/0.42f,0.0f,1.0f);
        const float reveal=arrival*arrival*(3.0f-2.0f*arrival);
        quad(px,py,pw,ph,0.008f,0.022f,0.030f,0.70f*reveal);
        // A single compile trace communicates arrival. Showing the entire
        // palette here made authored material roles read like a color legend.
        quad(px+18.0f,py+8.0f,(pw-36.0f)*reveal,2.0f,
            VisualIdentity::ComputedOutput.r,VisualIdentity::ComputedOutput.g,
            VisualIdentity::ComputedOutput.b,0.66f*reveal);
        text("ROOM "+std::to_string(state.roomIndex)+" / RECOMPILE",px+18,py+20,1.55f,0.82f,0.97f,1.0f,0.96f*reveal);
        const bool remoteChoice=state.multiplayer.enabled&&!state.multiplayer.authoritativeHost;
        text(remoteChoice?"HOST HOLDS RULE AUTHORITY":"RUN MUTATION / SELECT ONE",px+18,py+46,1.08f,0.62f,0.88f,0.94f,0.88f*reveal);
        const float cellW=(pw-24.0f)/3.0f;const std::string labels[3]={"SHOT","LUNGE","ATTACK"};
        const auto choice=[&](int item,float top,float height){
            const int track=item%3;const bool permanent=item>=3,selected=state.hud.menuSelection==item;
            const int level=permanent?state.progression.permanent.levels[track]:state.progression.run.temporaryLevels[track];
            const bool available=!permanent||(state.progression.permanent.tokens>0&&level<5);
            const float response=selected?clampf(state.cinematic.textInteraction,0,1):0.0f;
            const float cx=px+12+track*cellW+(cellW-4)*0.5f,cy=py+top+height*0.5f;
            const float tilt=selected&&!permanent?std::sin(state.upgradeMenu.presentationTime*2.5f+item*1.7f)*0.018f:0.0f;
            const VisualColor focus=permanent?VisualIdentity::AcidChartreuse:VisualIdentity::ElectricCyan;
            const VisualColor dim=VisualIdentity::MetallicTeal;
            const VisualColor color=available?focus:dim;
            rotatedQuad(cx,cy,cellW-6,height,tilt,color.r,color.g,color.b,(selected?0.15f+response*0.05f:0.035f)*reveal);
            rotatedQuad(cx,cy+height*0.5f-1,cellW-22,2,tilt,color.r,color.g,color.b,(selected?0.82f:0.24f)*reveal);
            const std::string& label=labels[track];const float labelScale=1.75f+response*0.06f;
            text(label,cx-label.size()*6*labelScale*0.5f,cy-18.0f,labelScale,selected?0.94f:0.72f,selected?1.0f:0.88f,1.0f,(selected?0.98f:0.72f)*reveal);
            const std::string detail=permanent?("PERM "+std::to_string(level)+"/5"):("RUN "+std::to_string(level)+" / NEXT "+std::to_string(std::min(12,level+1)));
            const float detailScale=0.92f;
            text(detail,cx-detail.size()*6*detailScale*0.5f,cy+12.0f,detailScale,color.r,color.g,color.b,(available?0.88f:0.48f)*reveal);
        };
        for(int i=0;i<3;++i)choice(i,66,76);
        text("PERMANENT MEMORY / TOKENS "+std::to_string(state.progression.permanent.tokens),px+18,py+158,1.08f,VisualIdentity::AcidChartreuse.r,VisualIdentity::AcidChartreuse.g,VisualIdentity::AcidChartreuse.b,0.86f*reveal);
        for(int i=3;i<6;++i)choice(i,184,66);
        text(remoteChoice?"WAITING FOR HOST":"ARROWS TUNE   ENTER / A LOCK",px+18,py+276,0.94f,0.58f,0.82f,0.88f,0.72f*reveal);
    } else if(state.uiPaused&&state.multiplayer.enabled){
        glPushMatrix();glScalef(menuUiScale,menuUiScale,1.0f);
        const float pw=360.0f,ph=116.0f,px=menuCanvasW-pw-12.0f,py=48.0f;
        quad(px,py,pw,ph,0.01f,0.03f,0.04f,0.16f);quad(px,py,pw,1,1,1,1,0.55f);quad(px,py+ph-1,pw,1,1,1,1,0.40f);quad(px,py,1,ph,1,1,1,0.42f);quad(px+pw-1,py,1,ph,1,1,1,0.42f);
        text("PAUSED",px+12,py+12,2.0f);const float pauseTilt=std::sin(state.time*2.4f)*0.018f,pulse=clampf(state.cinematic.textInteraction,0,1),buttonScale=2.45f+pulse*0.06f;rotatedQuad(px+pw*0.5f,py+63,pw-24,58,pauseTilt,0.16f,0.86f,1.0f,0.18f);text("RESUME",px+pw*0.5f-6*6*buttonScale*0.5f,py+54-3.5f*buttonScale,buttonScale,0.92f,1.0f,1.0f);
    }
    if(state.upgradeMenu.active||state.uiPaused)glPopMatrix();

    glMatrixMode(GL_MODELVIEW); glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}

void DesktopRenderer::drawDoorDataMosh(const GameState& state) const {
    if(!datamoshTexture_)glGenTextures(1,&datamoshTexture_);
    glBindTexture(GL_TEXTURE_2D,datamoshTexture_);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
    if(!state.doorTransition.active||state.doorTransition.progress<=0.018f){
        if(!datamoshFrameReady_||datamoshWidth_!=width_||datamoshHeight_!=height_){glCopyTexImage2D(GL_TEXTURE_2D,0,GL_RGB,0,0,width_,height_,0);datamoshWidth_=width_;datamoshHeight_=height_;datamoshFrameReady_=true;}
        else glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,width_,height_);
        return;
    }
    if(!datamoshFrameReady_)return;
    const float strength=clampf(state.doorTransition.progress,0,1),alpha=clampf(0.34f+strength*0.56f,0.34f,0.90f);
    const float mvX=clampf(state.doorTransition.frameMotion.x*width_*0.045f,-20.0f,20.0f)*(0.42f+strength*0.92f);
    const float mvY=clampf(-state.doorTransition.frameMotion.z*height_*0.030f,-24.0f,24.0f)*(0.42f+strength*0.92f);
    glDisable(GL_LIGHTING);glDisable(GL_DEPTH_TEST);glEnable(GL_TEXTURE_2D);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width_,0,height_,-1,1);glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    for(int pass=5;pass>=1;--pass){const float k=pass/5.0f;glColor4f(1,1,1,(0.07f+0.09f*k)*strength);glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(mvX*k,mvY*k);glTexCoord2f(1,0);glVertex2f(width_+mvX*k,mvY*k);glTexCoord2f(1,1);glVertex2f(width_+mvX*k,height_+mvY*k);glTexCoord2f(0,1);glVertex2f(mvX*k,height_+mvY*k);glEnd();}
    if(state.localSettings.particles){
        // The captured room remains whole at the threshold,
        // then its sampled cells peel through stable radial/depth trajectories.
        constexpr int columns=48,rows=27;
        const float release=1.0f-strength;
        const float eased=1.0f-(1.0f-release)*(1.0f-release);
        const float cellW=static_cast<float>(width_)/columns,cellH=static_cast<float>(height_)/rows;
        const auto noise=[](unsigned int value){value^=value>>16;value*=0x7feb352du;value^=value>>15;value*=0x846ca68bu;value^=value>>16;return static_cast<float>(value&0xffffu)/65535.0f;};
        glBegin(GL_QUADS);
        for(int row=0;row<rows;++row)for(int column=0;column<columns;++column){
            const unsigned int id=static_cast<unsigned int>(row*columns+column+1);
            const float x0=column*cellW,y0=row*cellH;
            const float centerX=x0+cellW*0.5f-width_*0.5f,centerY=y0+cellH*0.5f-height_*0.5f;
            const float invLength=1.0f/std::max(1.0f,std::sqrt(centerX*centerX+centerY*centerY));
            const float angle=(noise(id*17u)-0.5f)*1.1f;
            const float radialX=centerX*invLength,radialY=centerY*invLength;
            const float tangentX=-radialY,tangentY=radialX;
            const float travel=(42.0f+noise(id*31u)*150.0f)*eased;
            const float depth=(noise(id*47u)-0.5f)*0.72f;
            const float scale=clampf(1.0f+depth*eased,0.52f,1.42f);
            const float dx=(radialX*std::cos(angle)+tangentX*std::sin(angle))*travel+mvX*(0.35f+noise(id*61u));
            const float dy=(radialY*std::cos(angle)+tangentY*std::sin(angle))*travel+mvY*(0.35f+noise(id*73u));
            const float halfW=cellW*0.52f*scale,halfH=cellH*0.52f*scale,cx=x0+cellW*0.5f+dx,cy=y0+cellH*0.5f+dy;
            const float u0=static_cast<float>(column)/columns,u1=static_cast<float>(column+1)/columns;
            const float v0=static_cast<float>(row)/rows,v1=static_cast<float>(row+1)/rows;
            glColor4f(1,1,1,strength*(0.72f+0.24f*noise(id*89u)));
            glTexCoord2f(u0,v0);glVertex2f(cx-halfW,cy-halfH);glTexCoord2f(u1,v0);glVertex2f(cx+halfW,cy-halfH);glTexCoord2f(u1,v1);glVertex2f(cx+halfW,cy+halfH);glTexCoord2f(u0,v1);glVertex2f(cx-halfW,cy+halfH);
        }
        glEnd();
    }else{
        glColor4f(1,1,1,alpha);glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(mvX,mvY);glTexCoord2f(1,0);glVertex2f(width_+mvX,mvY);glTexCoord2f(1,1);glVertex2f(width_+mvX,height_+mvY);glTexCoord2f(0,1);glVertex2f(mvX,height_+mvY);glEnd();
    }
    glMatrixMode(GL_MODELVIEW);glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);glDisable(GL_BLEND);glDisable(GL_TEXTURE_2D);glEnable(GL_DEPTH_TEST);glEnable(GL_LIGHTING);
}

namespace {
// Reads back the fixed-function lighting state. Any GL error while reading marks
// the affected blocks unobserved instead of reporting stale or invented values.
lighting_evidence::GlFixedFunctionState observeGlFixedFunctionState(const char* capturePoint){
    using namespace lighting_evidence;
    GlFixedFunctionState gl{};
    while(glGetError()!=GL_NO_ERROR){}
    for(int i=0;i<GlLightCount;++i){
        GlLightState& light=gl.light[static_cast<std::size_t>(i)];const GLenum id=GL_LIGHT0+i;
        light.enabled=glIsEnabled(id)==GL_TRUE;
        glGetLightfv(id,GL_POSITION,light.position.data());glGetLightfv(id,GL_DIFFUSE,light.diffuse.data());glGetLightfv(id,GL_AMBIENT,light.ambient.data());
        glGetLightfv(id,GL_CONSTANT_ATTENUATION,&light.constantAttenuation);glGetLightfv(id,GL_LINEAR_ATTENUATION,&light.linearAttenuation);glGetLightfv(id,GL_QUADRATIC_ATTENUATION,&light.quadraticAttenuation);
    }
    gl.lights={glGetError()==GL_NO_ERROR,capturePoint};
    glGetFloatv(GL_LIGHT_MODEL_AMBIENT,gl.globalAmbient.data());
    gl.globalAmbientBlock={glGetError()==GL_NO_ERROR,capturePoint};
    gl.fogEnabled=glIsEnabled(GL_FOG)==GL_TRUE;glGetFloatv(GL_FOG_COLOR,gl.fogColor.data());glGetFloatv(GL_FOG_DENSITY,&gl.fogDensity);
    gl.fogBlock={glGetError()==GL_NO_ERROR,capturePoint};
    gl.normalize=glIsEnabled(GL_NORMALIZE)==GL_TRUE;gl.lighting=glIsEnabled(GL_LIGHTING)==GL_TRUE;gl.colorMaterial=glIsEnabled(GL_COLOR_MATERIAL)==GL_TRUE;gl.depthTest=glIsEnabled(GL_DEPTH_TEST)==GL_TRUE;
    gl.flagsBlock={glGetError()==GL_NO_ERROR,capturePoint};
    return gl;
}
}

void DesktopRenderer::draw(const GameState& state,const DeveloperCodecState* codec) const {
    updateCoastalWater(state);
    ++fpsFrames;const auto now=std::chrono::steady_clock::now();const float elapsed=std::chrono::duration<float>(now-fpsWindowStart).count();if(elapsed>=0.5f){displayedFps=fpsFrames/elapsed;fpsFrames=0;fpsWindowStart=now;}
    const auto atmosphere=resolvedAtmosphere(state);
    // The screen is an always-on physical object, not an effect that only
    // exists while attacking. Its idle floor is deliberately modest in day
    // light but keeps the phone present in night and sealed-room lighting.
    const float phoneIllumination=0.16f+state.phoneDisplay.localLightIntensity*0.58f+state.vacuum.power*0.42f+state.energy.dischargePositionAmount;
    const auto atmosphereInputs=render_contract::effectiveAtmosphereInputs(lightingControl_,{state.time,state.roomIndex,phoneIllumination});
    const auto roomPlan=room_environment::roomPlan(state.roomSeed,state.roomIndex);
    auto lightRig=room_lighting::roomLightRig(roomPlan.setting,roomPlan.form);
    const bool openSky=roomPlan.setting!=room_environment::RoomSetting::Sterile;
    if(openSky)lightRig.sunDirection=render_contract::celestialSunDirection(atmosphereInputs.time);
    const float objectiveProgress=state.requiredSouls>0?static_cast<float>(state.depositedSouls)/static_cast<float>(state.requiredSouls):0.0f;
    float captureActivity=0.0f;Vec3 capturePosition=state.phoneTransform.screenCenter;
    for(const auto& target:state.targets)if(target.alive&&target.slurpable){const float activity=clampf(std::max({target.vacuumPullAmount,target.soulVisual.latchAmount*0.72f,target.ingestProgress}),0.0f,1.0f);if(activity>captureActivity){captureActivity=activity;capturePosition=target.pos+Vec3{0,0.57f+target.soulVisual.verticalOffset,0};}}
    float completionAfterglow=0.0f,completionWeight=0.0f;Vec3 completionPosition{};
    for(const auto& particle:state.particles)if(particle.life>0.0f&&particle.maxLife>0.0f&&particle.material==ParticleMaterial::SignalResidue){
        const float age=particle.maxLife-particle.life;
        const float fresh=clampf(1.0f-age/1.15f,0.0f,1.0f);
        if(fresh>0.0f){completionAfterglow=std::max(completionAfterglow,fresh);completionPosition+=particle.pos*fresh;completionWeight+=fresh;}
    }
    if(captureActivity<=0.001f&&completionWeight>0.001f)capturePosition=completionPosition*(1.0f/completionWeight);
    const auto lightingResponse=scene_lighting_response::resolve({state.vacuum.power,state.energy.dischargePositionAmount,state.environmentVisual.latestShotAge,state.hud.criticalHitPulse,objectiveProgress,state.roomClear,captureActivity,completionAfterglow});
    glClearColor(atmosphere.background.r,atmosphere.background.g,atmosphere.background.b,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    applyCamera(state, static_cast<float>(width_)/static_cast<float>(height_));
    glEnable(GL_LIGHTING);glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);glEnable(GL_COLOR_MATERIAL);
    applyMaterial(semanticMaterial(SemanticSurface::Default));
    const float lightTileOrigin=static_cast<float>(state.topology.currentTileIndex)*ROOM_DEPTH;
    const auto resolvedLighting=room_lighting::resolveSceneLighting(atmosphere,lightRig,lightingResponse,state.phoneTransform.screenCenter,state.environmentVisual.latestShotOrigin,lightTileOrigin,capturePosition);
    const GLfloat ambient[]={resolvedLighting.ambient.r,resolvedLighting.ambient.g,resolvedLighting.ambient.b,1.0f};glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient);
    applyLight(GL_LIGHT0,resolvedLighting.sun);applyLight(GL_LIGHT1,resolvedLighting.fill);applyLight(GL_LIGHT2,resolvedLighting.phone);
    for(int i=0;i<3;++i)applyLight(GL_LIGHT3+i,resolvedLighting.localLights[static_cast<std::size_t>(i)]);
    applyLight(GL_LIGHT6,resolvedLighting.shot);applyLight(GL_LIGHT7,resolvedLighting.exit);
    glEnable(GL_FOG);const GLfloat fogColor[]={resolvedLighting.fog.color.r,resolvedLighting.fog.color.g,resolvedLighting.fog.color.b,1.0f};glFogfv(GL_FOG_COLOR,fogColor);glFogi(GL_FOG_MODE,GL_EXP2);glFogf(GL_FOG_DENSITY,resolvedLighting.fog.density);
    glEnable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glEnable(GL_LIGHTING); glEnable(GL_NORMALIZE);
    // Observation point: every world-pass light is set; HUD and later passes have not touched GL state yet.
    // Rigs with CeilingFixtures disable the global sun/fill (GL_LIGHT0/1); the enabled flags record that.
    lightingObservation_={true,atmosphere,atmosphereInputs,lightRig,
        {state.vacuum.power,state.energy.dischargePositionAmount,state.environmentVisual.latestShotAge,state.hud.criticalHitPulse,objectiveProgress,state.roomClear},lightingResponse,
        observeGlFixedFunctionState("DesktopRenderer::draw:world-pass-lights-set")};
    const bool cheapVisuals=state.localSettings.graphicsPreset<=0;
    const auto actorVisible=[&](const Vec3& position){const Vec3 delta=position-state.camera.pos;const float maxDist=cheapVisuals?38.0f:55.0f;return lengthSq(delta)<maxDist*maxDist&&dot3(delta,state.camera.forward)>-8.0f;};
    if(openSky){
        const Vec3 forward=state.camera.forward;
        Vec3 right=normalized(cross3(forward,{0,1,0}));if(lengthSq(right)<0.01f)right={1,0,0};
        const Vec3 up=normalized(cross3(right,forward));
        const float distance=VisualIdentity::CameraFarPlane*0.88f;
        const float halfHeight=distance*0.70f,halfWidth=halfHeight*static_cast<float>(width_)/static_cast<float>(height_);
        const Vec3 center=state.camera.pos+forward*distance;
        const VisualColor zenith{
            clampf(atmosphere.background.r*0.44f+atmosphere.sun.r*0.10f,0.0f,1.0f),
            clampf(atmosphere.background.g*0.54f+atmosphere.sun.g*0.10f,0.0f,1.0f),
            clampf(atmosphere.background.b*0.72f+atmosphere.sun.b*0.10f,0.0f,1.0f)};
        const VisualColor horizon{
            clampf(atmosphere.background.r*0.74f+atmosphere.fog.r*0.26f+0.08f,0.0f,1.0f),
            clampf(atmosphere.background.g*0.78f+atmosphere.fog.g*0.22f+0.07f,0.0f,1.0f),
            clampf(atmosphere.background.b*0.86f+atmosphere.fog.b*0.14f+0.03f,0.0f,1.0f)};
        glDisable(GL_LIGHTING);glDisable(GL_FOG);glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);
        glBegin(GL_QUADS);
        glColor3f(zenith.r,zenith.g,zenith.b);glVertex3f((center-right*halfWidth+up*halfHeight).x,(center-right*halfWidth+up*halfHeight).y,(center-right*halfWidth+up*halfHeight).z);
        glColor3f(zenith.r,zenith.g,zenith.b);glVertex3f((center+right*halfWidth+up*halfHeight).x,(center+right*halfWidth+up*halfHeight).y,(center+right*halfWidth+up*halfHeight).z);
        glColor3f(horizon.r,horizon.g,horizon.b);glVertex3f((center+right*halfWidth-up*halfHeight).x,(center+right*halfWidth-up*halfHeight).y,(center+right*halfWidth-up*halfHeight).z);
        glColor3f(horizon.r,horizon.g,horizon.b);glVertex3f((center-right*halfWidth-up*halfHeight).x,(center-right*halfWidth-up*halfHeight).y,(center-right*halfWidth-up*halfHeight).z);
        glEnd();
        glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);glEnable(GL_FOG);glEnable(GL_LIGHTING);
    }
    // A restrained physical disc anchors the outdoor lighting. It is drawn as
    // part of the distant sky and is then naturally occluded by world geometry.
    if(openSky){
        const Vec3 celestial=normalized(lightRig.sunDirection);
        const float daylight=render_contract::celestialDaylight(atmosphereInputs.time);
        const bool sunAbove=celestial.y>-0.04f;
        const Vec3 direction=sunAbove?celestial:celestial*-1.0f;
        const Vec3 center=state.camera.pos+direction*82.0f;
        Vec3 right=normalized(cross3(state.camera.forward,{0,1,0}));if(lengthSq(right)<0.01f)right={1,0,0};
        const Vec3 up=normalized(cross3(right,state.camera.forward));
        const float radius=sunAbove?(1.65f+0.45f*(1.0f-daylight)):0.82f;
        const VisualColor color=sunAbove?atmosphere.sun:VisualColor{0.43f,0.52f,0.66f};
        glDisable(GL_LIGHTING);glDisable(GL_FOG);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glDepthMask(GL_FALSE);
        glBegin(GL_TRIANGLE_FAN);glColor4f(color.r,color.g,color.b,sunAbove?0.82f:0.42f);glVertex3f(center.x,center.y,center.z);
        constexpr int celestialSegments=28;for(int i=0;i<=celestialSegments;++i){const float a=2.0f*PI*static_cast<float>(i)/celestialSegments;const Vec3 edge=center+right*(std::cos(a)*radius)+up*(std::sin(a)*radius);glColor4f(color.r,color.g,color.b,0.0f);glVertex3f(edge.x,edge.y,edge.z);}glEnd();
        glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_FOG);glEnable(GL_LIGHTING);
    }
    for(int tile=state.topology.currentTileIndex-ROOM_VISUAL_HORIZON;tile<=state.topology.currentTileIndex+ROOM_VISUAL_HORIZON;++tile)drawRoomTile(state,tile,lightingResponse);

    // The secret room is deliberately disconnected from the repeating corridor:
    // a tiny, cheap collection of boxes makes it feel like found backstage space.
    if(state.secretTv.available){
        const float knock=clampf(state.secretTv.knockPulse,0.0f,1.0f);
        const float breathe=0.04f+0.035f*std::sin(state.time*2.1f);
        const float push=knock*(0.10f+0.018f*std::sin(state.time*41.0f));
        const float alpha=(state.secretTv.broken?0.12f:0.25f)+knock*0.28f;
        const Vec3 wallCenter=state.secretTv.entrancePos+Vec3{0.0f,1.22f,0.0f};
        const Vec3 pushedCenter=wallCenter+state.secretTv.entranceNormal*push;
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
        drawBox(pushedCenter,{0.08f+push*0.42f,2.30f+breathe+knock*0.10f,2.20f+breathe+knock*0.08f},0,0,0,VisualIdentity::TvMembrane.r,VisualIdentity::TvMembrane.g,VisualIdentity::TvMembrane.b,alpha);
        if(knock>0.025f)drawBox(pushedCenter+state.secretTv.entranceNormal*(0.07f+push*0.5f),{0.03f,1.82f+knock*0.24f,1.72f+knock*0.20f},0,0,0,VisualIdentity::ElectricCyan.r,VisualIdentity::ElectricCyan.g,VisualIdentity::ElectricCyan.b,0.18f*knock);
        glDepthMask(GL_TRUE);glDisable(GL_BLEND);
    }
    if(state.player.inSecretRoom){
        drawBox({40.4f,-0.04f,0},{7.4f,0.08f,6.4f},0,0,0,VisualIdentity::SecretFloor.r,VisualIdentity::SecretFloor.g,VisualIdentity::SecretFloor.b);
        drawBox({36.7f,2.4f,0},{0.12f,4.8f,6.4f},0,0,0,VisualIdentity::SecretWall.r,VisualIdentity::SecretWall.g,VisualIdentity::SecretWall.b);
        drawBox({40.4f,2.4f,-3.2f},{7.4f,4.8f,0.12f},0,0,0,VisualIdentity::SecretWall.r,VisualIdentity::SecretWall.g,VisualIdentity::SecretWall.b);
        drawBox({40.4f,2.4f,3.2f},{7.4f,4.8f,0.12f},0,0,0,VisualIdentity::SecretWall.r,VisualIdentity::SecretWall.g,VisualIdentity::SecretWall.b);
        drawBox({42.25f,0.70f,0},{1.75f,1.35f,0.82f},0,-1.5708f,0,VisualIdentity::SecretBlack.r,VisualIdentity::SecretBlack.g,VisualIdentity::SecretBlack.b);
        float phoneProximity=0.0f;const Vec3 tvPosition{41.82f,0.78f,0};
        const auto includePhone=[&](const PlayerState& player,bool active){if(!active||!player.inSecretRoom)return;const float distance=length(player.pos-tvPosition);phoneProximity=std::max(phoneProximity,1.0f-clampf(distance/6.0f,0.0f,1.0f));};
        includePhone(state.player,true);if(state.multiplayer.enabled)for(const auto& peer:state.multiplayer.peers)includePhone(peer.player,peer.active);
        drawSecretTvScreen(state,phoneProximity);
        drawBox({41.35f,0.18f,-0.80f},{1.8f,0.055f,0.055f},0,0.18f,0,VisualIdentity::SecretCable.r,VisualIdentity::SecretCable.g,VisualIdentity::SecretCable.b);
        drawBox({41.45f,0.16f,0.76f},{2.1f,0.045f,0.045f},0,-0.22f,0,VisualIdentity::SecretCable.r,VisualIdentity::SecretCable.g,VisualIdentity::SecretCable.b);
    }

    // A pause menu overlays the live world, so its frozen actors still own their
    // shadows. Only presentations that replace gameplay suppress world shadows.
    const bool menuPresentation=(((!state.started&&!state.dead)||state.dead||state.cinematic.introActive)&&!state.multiplayer.enabled&&!state.upgradeMenu.active);
    const auto shadowQuality=render_contract::shadowQualityFor(state.localSettings.graphicsPreset,state.localSettings.shadows,true);
    if(shadowQuality==render_contract::ShadowQuality::Cheap){
    glDisable(GL_LIGHTING);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
    if(!menuPresentation&&!state.camera.firstPerson&&state.player.alive)drawGroundShadow(state.phoneTransform.position,0.25f,0.18f,0.95f,0.20f,shadowReceiverHeight(state,state.player.pos.x,state.player.pos.z,0.12f));
    if(!menuPresentation&&state.multiplayer.enabled)for(const auto& peer:state.multiplayer.peers)if(peer.active&&peer.playerId!=state.multiplayer.localPlayerId&&peer.player.alive)drawGroundShadow(peer.player.pos,0.25f,0.18f,0.95f,0.20f,shadowReceiverHeight(state,peer.player.pos.x,peer.player.pos.z,0.12f));
    if(!menuPresentation){
    const float shadowTileOrigin=static_cast<float>(state.topology.currentTileIndex)*ROOM_DEPTH;
    for(int offset=-1;offset<=1;++offset)for(auto target:state.targets)if(target.alive){target.pos.z=shadowTileOrigin+static_cast<float>(offset)*ROOM_DEPTH+(target.pos.z-std::floor((target.pos.z+ROOM_DEPTH*0.5f)/ROOM_DEPTH)*ROOM_DEPTH);if(!actorVisible(target.pos))continue;const float receiver=shadowReceiverHeight(state,target.pos.x,target.pos.z,0.14f);if(!target.slurpable){drawGroundShadow(target.pos,0.30f*target.scale,0.22f*target.scale,1.1f*target.scale,0.13f,receiver);drawGroundShadow(target.pos,0.19f*target.scale,0.12f*target.scale,0.08f*target.scale,0.16f+target.visualReaction.commitment*0.05f,receiver);}else if(target.soulVisual.visible&&target.soulCubeAmount>0.001f)drawGroundShadow(target.pos,0.26f*target.scale,0.26f*target.scale,0.72f*target.scale,0.16f,receiver);}
    for(const auto& flower:state.flowers)if(flower.active){const Vec3 p{flower.pos.x,flower.pos.y,flower.pos.z+shadowTileOrigin};drawGroundShadow(p,0.27f,0.27f,0.72f,0.16f,shadowReceiverHeight(state,p.x,p.z,0.12f));}
    for(const auto& bullet:state.bullets)if(bullet.alive){const float radius=0.40f*(bullet.brute?1.7f:1.0f);drawGroundShadow(bullet.pos,radius,radius,radius*2.0f,0.14f,shadowReceiverHeight(state,bullet.pos.x,bullet.pos.z,0.10f));}
    // Static room geometry already provides the receiving floor and its own
    // lighting. Projecting every collider (including room-scale walls) onto the
    // floor created large overlapping black sheets unrelated to visible casters.
    }
    glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_LIGHTING);}
    else if(shadowQuality==render_contract::ShadowQuality::Directional&&!menuPresentation){
        Vec3 sun=lightRig.sunDirection;
        if(sun.y<0.10f)sun={-sun.x,std::max(0.18f,-sun.y),-sun.z};
        const float shadowMatrix[16]={1,0,0,0,-sun.x/sun.y,0,-sun.z/sun.y,0,0,0,1,0,0.006f,0.012f,0.005f,1};
        glStencilMask(0xff);glClearStencil(0);glClear(GL_STENCIL_BUFFER_BIT);glEnable(GL_STENCIL_TEST);glStencilFunc(GL_ALWAYS,1,0xff);glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
        glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);glDepthMask(GL_FALSE);glDisable(GL_DEPTH_TEST);glPushMatrix();glMultMatrixf(shadowMatrix);
        if(!state.camera.firstPerson&&state.player.alive){if(phoneShadowList_)drawStaticModel(phoneShadowList_,state.phoneTransform.position,state.phoneVisual.bodyScale,state.phoneTransform.orientation);else drawBox(state.phoneTransform.position,{PHONE_BODY_WIDTH,PHONE_BODY_HEIGHT,PHONE_BODY_DEPTH},state.phoneTransform.orientation,0,0,0);}
        if(state.multiplayer.enabled)for(const auto& peer:state.multiplayer.peers)if(peer.active&&peer.playerId!=state.multiplayer.localPlayerId&&peer.player.alive){if(phoneShadowList_)drawStaticModel(phoneShadowList_,peer.phoneTransform.position,peer.phoneVisual.bodyScale,peer.phoneTransform.orientation);else drawBox(peer.phoneTransform.position,{PHONE_BODY_WIDTH,PHONE_BODY_HEIGHT,PHONE_BODY_DEPTH},peer.phoneTransform.orientation,0,0,0);}
        const float shadowTileOrigin=static_cast<float>(state.topology.currentTileIndex)*ROOM_DEPTH;
        const auto roomSetting=room_environment::roomPlan(state.roomSeed,state.roomIndex).setting;
        for(int offset=-1;offset<=1;++offset)for(auto target:state.targets)if(target.alive){target.pos.z=shadowTileOrigin+static_cast<float>(offset)*ROOM_DEPTH+(target.pos.z-std::floor((target.pos.z+ROOM_DEPTH*0.5f)/ROOM_DEPTH)*ROOM_DEPTH);if(!actorVisible(target.pos))continue;if(!target.slurpable){if(humanModel_.valid())drawHumanModel(target,state.time,roomSetting,true);else drawProceduralHumanDesktop(target,state.time,0,0,0);}else if(target.soulVisual.visible&&target.soulCubeAmount>0.001f){const auto& sv=target.soulVisual;const float cube=0.72f*0.78f*target.scale*sv.morphScale;drawBox(target.pos+Vec3{0,0.57f+sv.verticalOffset,0},{cube*sv.scale.x,cube*sv.scale.y,cube*sv.scale.z},0,sv.rotationY,0,0,0,0);}}
        for(const auto& flower:state.flowers)if(flower.active)drawBox({flower.pos.x,flower.pos.y,flower.pos.z+shadowTileOrigin},{0.54f,0.22f,0.54f},0,flower.rotationY,0,0,0,0);
        for(const auto& bullet:state.bullets)if(bullet.alive){const float size=0.72f*1.12f*(bullet.brute?1.7f:1.0f);drawBox(bullet.pos,{size,size,size},bullet.spin*1.2f,bullet.spin*1.7f,bullet.spin*0.9f,0,0,0);}
        glPopMatrix();glEnable(GL_DEPTH_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glStencilMask(0);glStencilFunc(GL_EQUAL,1,0xff);glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
        glDisable(GL_LIGHTING);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);drawBox({0,0.014f,shadowTileOrigin},{ROOM_WIDTH,0.008f,ROOM_DEPTH*3.0f},0,0,0,0.012f,0.018f,0.022f,0.24f);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_LIGHTING);glDisable(GL_STENCIL_TEST);
        // Directional projection owns the broad silhouette. A quiet support-
        // aware contact patch closes the gap on rocks, slopes and transition
        // tiles that are not represented by the old floor-only receiver.
        glDisable(GL_LIGHTING);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
        if(!state.camera.firstPerson&&state.player.alive)drawGroundShadow(state.phoneTransform.position,0.18f,0.13f,0.10f,0.13f,shadowReceiverHeight(state,state.player.pos.x,state.player.pos.z,0.12f));
        for(int offset=-1;offset<=1;++offset)for(auto target:state.targets)if(target.alive){target.pos.z=shadowTileOrigin+static_cast<float>(offset)*ROOM_DEPTH+(target.pos.z-std::floor((target.pos.z+ROOM_DEPTH*0.5f)/ROOM_DEPTH)*ROOM_DEPTH);if(!actorVisible(target.pos))continue;drawGroundShadow(target.pos,0.18f*target.scale,0.12f*target.scale,0.08f,0.11f,shadowReceiverHeight(state,target.pos.x,target.pos.z,0.14f));}
        glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_LIGHTING);
    }

    if (state.phoneVisual.visible) {
        const Vec3 phonePos=state.phoneTransform.position;
        const auto& pv=state.phoneVisual;
        const Quat phoneOrientation=state.phoneTransform.orientation;
        {const ScopedMaterial material(semanticMaterial(SemanticSurface::PhoneMetal,VisualIdentity::PhoneBody));
        if(phoneModelList_) drawStaticModel(phoneModelList_,phonePos,pv.bodyScale,phoneOrientation);
        else drawBox(phonePos,{PHONE_BODY_WIDTH*pv.bodyScale.x,PHONE_BODY_HEIGHT*pv.bodyScale.y,PHONE_BODY_DEPTH},phoneOrientation,VisualIdentity::PhoneBody.r,VisualIdentity::PhoneBody.g,VisualIdentity::PhoneBody.b);}
        const float glow=std::min(1.0f,0.45f+pv.screenGlow*0.36f);
        {const ScopedMaterial material(semanticMaterial(SemanticSurface::PhoneGlass,{VisualIdentity::PhoneEmission.r*glow,VisualIdentity::PhoneEmission.g*glow,VisualIdentity::PhoneEmission.b*glow}));
        drawBox(state.phoneTransform.screenCenter,{PHONE_SCREEN_WIDTH*PHONE_DISPLAY_INSET_X*pv.screenScale.x,PHONE_SCREEN_HEIGHT*PHONE_DISPLAY_INSET_Y*pv.screenScale.y,PHONE_SCREEN_DEPTH*0.30f},phoneOrientation,VisualIdentity::PhoneEmission.r*glow,VisualIdentity::PhoneEmission.g*glow,VisualIdentity::PhoneEmission.b*glow);}
        if(state.phoneDisplay.mode!=PhoneDisplayMode::Off&&state.phoneDisplay.mode!=PhoneDisplayMode::Death)
            drawPhoneDisplayTexture(state);
        const float idleScreenAura=0.055f+(openSky?1.0f-render_contract::celestialDaylight(atmosphereInputs.time):0.42f)*0.085f;
        glDisable(GL_LIGHTING);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glDepthMask(GL_FALSE);
        drawBox(state.phoneTransform.screenCenter+state.phoneTransform.screenNormal*0.010f,{PHONE_SCREEN_WIDTH*PHONE_DISPLAY_INSET_X*1.055f,PHONE_SCREEN_HEIGHT*PHONE_DISPLAY_INSET_Y*1.035f,PHONE_SCREEN_DEPTH*0.18f},phoneOrientation,state.phoneDisplay.emissionColor.x,state.phoneDisplay.emissionColor.y,state.phoneDisplay.emissionColor.z,idleScreenAura);
        glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_LIGHTING);
        const float captureAura=clampf(pv.captureContactAmount*0.72f+std::max(0.0f,pv.ingestBulge)*0.58f+captureActivity*0.34f+completionAfterglow*0.42f,0.0f,1.0f);
        if(captureAura>0.015f){glDisable(GL_LIGHTING);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glDepthMask(GL_FALSE);const float pulse=0.92f+0.08f*std::sin(state.time*24.0f);drawBox(state.phoneTransform.screenCenter+state.phoneTransform.screenNormal*0.012f,{PHONE_SCREEN_WIDTH*PHONE_DISPLAY_INSET_X*(1.04f+captureAura*0.12f)*pulse,PHONE_SCREEN_HEIGHT*PHONE_DISPLAY_INSET_Y*(1.03f+captureAura*0.10f)*pulse,PHONE_SCREEN_DEPTH*0.22f},phoneOrientation,VisualIdentity::ElectricCyan.r,VisualIdentity::ElectricCyan.g,VisualIdentity::ElectricCyan.b,0.14f+captureAura*0.24f);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_LIGHTING);}
    }
    if(state.multiplayer.enabled)for(const auto& peer:state.multiplayer.peers)if(peer.active&&peer.playerId!=state.multiplayer.localPlayerId&&peer.player.alive){
        const auto& pv=peer.phoneVisual;
        {const ScopedMaterial material(semanticMaterial(SemanticSurface::PhoneMetal,{0.32f,0.86f,1.0f}));
        if(phoneModelList_)drawStaticModel(phoneModelList_,peer.phoneTransform.position,pv.bodyScale,peer.phoneTransform.orientation);else drawBox(peer.phoneTransform.position,{PHONE_BODY_WIDTH,PHONE_BODY_HEIGHT,PHONE_BODY_DEPTH},peer.phoneTransform.orientation,0.32f,0.86f,1.0f);}
        {const ScopedMaterial material(semanticMaterial(SemanticSurface::PhoneGlass,{0.05f,0.55f,0.78f}));
        drawBox(peer.phoneTransform.screenCenter,{PHONE_SCREEN_WIDTH*PHONE_DISPLAY_INSET_X,PHONE_SCREEN_HEIGHT*PHONE_DISPLAY_INSET_Y,PHONE_SCREEN_DEPTH*0.30f},peer.phoneTransform.orientation,0.05f,0.55f,0.78f);}
    }

    const MeleeVisualState& melee=state.meleeVisual;
    if(melee.visualTimer>0.0f && !melee.locomotionLunge){
        const float t=1.0f-clampf(melee.visualTimer/std::max(0.001f,melee.visualDuration),0.0f,1.0f);
        const float fade=1.0f-t, hitBoost=melee.visualHit?1.25f:0.72f;
        glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        const Quat slashQ=quatAxisAngle({0,1,0},state.player.yaw)*quatAxisAngle({1,0,0},PI*0.5f)*quatAxisAngle({0,0,1},-PI*0.28f+t*PI*1.15f);
        fxRibbon(melee.origin+melee.direction*(0.66f+0.22f*t),slashQ,{(0.75f+t*0.72f)*hitBoost,(0.75f+t*0.72f)*hitBoost,1},0,PI*1.35f,24,0.494f,0.546f,VisualIdentity::ElectricMagenta.r,VisualIdentity::ElectricMagenta.g,VisualIdentity::ElectricMagenta.b,fade*0.66f);
        const Vec3 delta=melee.impact-melee.origin; const float len=std::max(0.3f,length(delta));
        const Vec3 mid=melee.origin+delta*0.46f; const float yaw=std::atan2(delta.x,delta.z); const float pitch=-std::asin(clampf(delta.y/std::max(len,0.001f),-1.0f,1.0f));
        const Quat streakQ=quatAxisAngle({0,1,0},yaw)*quatAxisAngle({1,0,0},pitch);
        fxStreak(mid,streakQ,len*(0.70f+std::sin(t*PI)*0.18f),0.11f*(1+std::sin(t*PI)*1.2f),VisualIdentity::ElectricCyan.r,VisualIdentity::ElectricCyan.g,VisualIdentity::ElectricCyan.b,fade*0.44f);
        fxRibbon(melee.impact,{}, {(0.45f+t*1.45f)*hitBoost,(0.45f+t*1.45f)*hitBoost,1},0,PI*2,24,0.318f,0.362f,VisualIdentity::AcidChartreuse.r,VisualIdentity::AcidChartreuse.g,VisualIdentity::AcidChartreuse.b,melee.visualHit?fade*0.82f:fade*0.24f);
        glDisable(GL_BLEND); glEnable(GL_LIGHTING);
    }

    struct TranslucentSoulDraw { Vec3 center; Vec3 scale; float rotationY; VisualColor color; float opacity; float distanceSquared; };
    std::array<TranslucentSoulDraw,TARGET_COUNT*3> translucentSouls{};
    int translucentSoulCount=0;
    const float tileOrigin=static_cast<float>(state.topology.currentTileIndex)*ROOM_DEPTH;
    for(int offset=-1;offset<=1;++offset)for (auto target:state.targets) if (target.alive) {
        Vec3 p=target.pos; p.z=tileOrigin+static_cast<float>(offset)*ROOM_DEPTH+(target.pos.z-std::floor((target.pos.z+ROOM_DEPTH*0.5f)/ROOM_DEPTH)*ROOM_DEPTH);
        if(!actorVisible(p))continue;
        const float mirrorShift=p.z-target.pos.z;
        target.tetherAnchor.z+=mirrorShift;target.tetherDestination.z+=mirrorShift;target.latchPoint.z+=mirrorShift;
        target.pos = p;
        if (!target.slurpable) {
            const auto setting=room_environment::roomPlan(state.roomSeed,state.roomIndex).setting;const VisualColor damageColor=humanDamageSurfaceColor(enemyShellColor(target.brute,target.attackVariant,state.roomIndex),setting,target.armor,target.brute?4.0f:2.0f,target.slurpable,target.hitFlash);
            if(humanModel_.valid())drawHumanModel(target,state.time,setting);else drawProceduralHumanDesktop(target,state.time,damageColor.r,damageColor.g,damageColor.b);
        }
        if (target.slurpable && target.soulCubeAmount > 0.001f) {
            const auto& sv=target.soulVisual;
            if (!sv.visible) continue;
            const Vec3 soulCenter=p+Vec3{0,0.57f+sv.verticalOffset,0};
            drawSoulFlesh(target,soulCenter);
            const float cube=0.72f*0.78f*target.scale*sv.morphScale;
            const Vec3 delta=soulCenter-state.camera.pos;
            const VisualColor prismColor={sv.color.r*0.48f+VisualIdentity::SoulPrism.r*0.52f,sv.color.g*0.48f+VisualIdentity::SoulPrism.g*0.52f,sv.color.b*0.48f+VisualIdentity::SoulPrism.b*0.52f};
            const float readableShellOpacity=clampf(sv.shellOpacity*0.64f,0.08f,0.42f);
            translucentSouls[translucentSoulCount++]={soulCenter,{cube*sv.scale.x,cube*sv.scale.y,cube*sv.scale.z},sv.rotationY,prismColor,readableShellOpacity,delta.x*delta.x+delta.y*delta.y+delta.z*delta.z};
        }
        // Let the final startled utterance bridge the physical body and its
        // newly exposed data substrate instead of vanishing on the kill tick.
        drawEnemyReactionBillboard(state,target);
    }
    std::sort(translucentSouls.begin(),translucentSouls.begin()+translucentSoulCount,[](const auto& a,const auto& b){return a.distanceSquared>b.distanceSquared;});
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    // Draw one camera-facing surface of each convex shell. With culling
    // disabled, several cube faces compound alpha and produce false opacity.
    glEnable(GL_CULL_FACE); glCullFace(GL_BACK);
    for(int i=0;i<translucentSoulCount;++i){const auto& soul=translucentSouls[i];drawBox(soul.center,soul.scale,0,soul.rotationY,0,soul.color.r,soul.color.g,soul.color.b,soul.opacity);}
    glDisable(GL_CULL_FACE); glDepthMask(GL_TRUE); glDisable(GL_BLEND);
    for(int offset=-ROOM_VISUAL_HORIZON;offset<=ROOM_VISUAL_HORIZON;++offset)for (int captureIndex=0;captureIndex<state.requiredSouls;++captureIndex) {
        const auto& capture=state.captures[captureIndex];
        Vec3 p=capture.pos; p.z+=tileOrigin+static_cast<float>(offset)*ROOM_DEPTH;
        drawBox(p+Vec3{0,0,-0.04f},{0.72f,0.72f,0.06f},0,0,0,VisualIdentity::MetallicTeal.r*0.74f,VisualIdentity::MetallicTeal.g*0.74f,VisualIdentity::MetallicTeal.b*0.74f);
        drawBox(p,{0.52f,0.52f,0.08f},0,0,0,0.02f,0.03f,0.04f);
        if(capture.filled) drawBox(p+Vec3{0,0,0.12f},{0.36f,0.36f,0.36f},state.time*1.5f,state.time*2.0f,state.time,VisualIdentity::SoulBase.r,VisualIdentity::SoulBase.g,VisualIdentity::SoulBase.b);
    }
    // Opaque soul projectiles resolve first. Power flowers then retain their
    // own readable shell/core/glyph hierarchy instead of being visually
    // replaced by a coincident brown cube.
    for (const auto& bullet:state.bullets) if (bullet.alive) {
        const float size=0.72f*1.12f*(bullet.brute?1.7f:1.0f);
        drawBox(bullet.pos,{size,size,size},bullet.spin*1.2f,bullet.spin*1.7f,bullet.spin*0.9f,VisualIdentity::SoulBase.r,VisualIdentity::SoulBase.g,VisualIdentity::SoulBase.b,0.68f);
    }
    for(const auto& flower:state.flowers) if(flower.active){
        for(int offset=-ROOM_VISUAL_HORIZON;offset<=ROOM_VISUAL_HORIZON;++offset){
            const Vec3 center{flower.pos.x,flower.pos.y,flower.pos.z+static_cast<float>(state.topology.currentTileIndex+offset)*ROOM_DEPTH};
            constexpr char flowerGlyphs[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            const unsigned int phase=static_cast<unsigned int>(state.time*3.0f),positionHash=static_cast<unsigned int>(std::abs(flower.pos.x)*97.0f+std::abs(flower.pos.z)*193.0f)+static_cast<unsigned int>(state.roomIndex)*17u;
            unsigned int symbolHash=(phase+positionHash)*1664525u+1013904223u;
            const auto rows=bitmapGlyph(flowerGlyphs[(symbolHash>>16)%36u]);
            const Vec3 toCamera=normalized(state.camera.pos-center);const Vec3 glyphRight=normalized(cross3({0,1,0},toCamera));const Vec3 glyphUp=normalized(cross3(toCamera,glyphRight));const float glyphYaw=std::atan2(toCamera.x,toCamera.z);
            glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
            glEnable(GL_CULL_FACE);glCullFace(GL_BACK);
            drawBox(center,{0.72f,0.72f,0.72f},0,flower.rotationY,0,VisualIdentity::Flower.r,VisualIdentity::Flower.g,VisualIdentity::Flower.b,0.36f);
            glDisable(GL_CULL_FACE);glDepthMask(GL_TRUE);glDisable(GL_BLEND);
            drawBox(center,{0.29f,0.29f,0.29f},flower.rotationY*0.42f,-flower.rotationY*0.61f,0,VisualIdentity::FlowerCore.r,VisualIdentity::FlowerCore.g,VisualIdentity::FlowerCore.b);
            glDisable(GL_LIGHTING);for(int row=0;row<7;++row)for(int col=0;col<5;++col)if(rows[row]&(1u<<(4-col))){const Vec3 pixel=center+toCamera*0.375f+glyphRight*((static_cast<float>(col)-2.0f)*0.072f)+glyphUp*((3.0f-static_cast<float>(row))*0.072f);drawBox(pixel,{0.058f,0.058f,0.018f},0,glyphYaw,0,0.94f,1.0f,0.86f);}glEnable(GL_LIGHTING);
        }
    }
    if(completionAfterglow>0.015f&&completionWeight>0.001f){
        const float release=1.0f-completionAfterglow;
        const float span=0.34f+release*0.82f;
        const Vec3 echoCenter=capturePosition+Vec3{0,0.035f-release*0.08f,0};
        glDisable(GL_LIGHTING);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE);glDepthMask(GL_FALSE);
        drawBox(echoCenter,{span,0.018f,span},0,release*PI*0.30f,0,VisualIdentity::ElectricCyan.r,VisualIdentity::ElectricCyan.g,VisualIdentity::ElectricCyan.b,completionAfterglow*0.16f);
        drawBox(echoCenter,{span*0.64f,0.022f,span*0.64f},0,-release*PI*0.46f,0,VisualIdentity::ElectricMagenta.r,VisualIdentity::ElectricMagenta.g,VisualIdentity::ElectricMagenta.b,completionAfterglow*0.11f);
        glDepthMask(GL_TRUE);glDisable(GL_BLEND);glEnable(GL_LIGHTING);
    }
    if(state.localSettings.particles)for(const auto& particle:state.particles) if(particle.life>0.0f) {
        const float t=particle.maxLife>0.0f?clampf(particle.life/particle.maxLife,0.0f,1.0f):0.0f;
        const float size=particle.size*t;
        const VisualColor color=particleMaterialColor(particle.material,room_environment::roomPlan(state.roomSeed,state.roomIndex).setting,t);
        if(particle.material==ParticleMaterial::SignalResidue&&particleSurfaceSettled(particle)){
            const float spread=std::max(0.035f,size);
            const unsigned int seed=(static_cast<unsigned int>(std::fabs(particle.pos.x)*7919.0f)^
                static_cast<unsigned int>(std::fabs(particle.pos.z)*4733.0f)^
                static_cast<unsigned int>(std::fabs(particle.pos.y)*104729.0f))*2654435761u;
            auto hashf=[seed](unsigned int salt){
                unsigned int h=(seed^(salt*2246822519u))*3266489917u;h^=h>>15;h*=2654435761u;h^=h>>13;
                return static_cast<float>(h&0xFFFFu)/65535.0f;
            };
            const float baseRotation=hashf(1u)*PI*2.0f;
            const float dr=color.r*0.30f,dg=color.g*0.22f,db=color.b*0.34f;
            const Vec3 surfaceNormal=particleSurfaceNormal(particle);
            Vec3 tangent=cross3(std::abs(surfaceNormal.y)>0.92f?Vec3{1,0,0}:Vec3{0,1,0},surfaceNormal);
            tangent=normalized(tangent);const Vec3 bitangent=normalized(cross3(surfaceNormal,tangent));
            const Vec3 tiltAxis=normalized(cross3(Vec3{0,1,0},surfaceNormal));
            const float tiltAngle=std::acos(clampf(surfaceNormal.y,-1.0f,1.0f));
            const Quat tilt=lengthSq(tiltAxis)>0.0001f?quatAxisAngle(tiltAxis,tiltAngle):Quat{};
            // irregular dark fluid stain: several overlapping, unevenly sized lobes
            const int lobeCount=4;
            for(int lobe=0;lobe<lobeCount;++lobe){
                const float lobeAngle=baseRotation+static_cast<float>(lobe)*(PI*0.5f)+(hashf(10u+static_cast<unsigned int>(lobe))-0.5f)*1.1f;
                const float lobeRadius=spread*(0.30f+0.55f*hashf(20u+static_cast<unsigned int>(lobe)));
                const Vec3 lobeOffset=tangent*(std::cos(lobeAngle)*lobeRadius)
                    +bitangent*(std::sin(lobeAngle)*lobeRadius)+surfaceNormal*0.005f;
                const float lobeW=spread*(0.95f+0.65f*hashf(30u+static_cast<unsigned int>(lobe)));
                const float lobeD=spread*(0.75f+0.60f*hashf(40u+static_cast<unsigned int>(lobe)));
                const Quat orientation=quatAxisAngle(surfaceNormal,lobeAngle)*tilt;
                drawBox(particle.pos+lobeOffset,{lobeW,0.010f,lobeD},orientation,dr,dg,db,0.70f*t);
            }
            // restrained embedded signal fragments: small, dim glints, not a bright emblem
            const int glintCount=3;
            for(int g=0;g<glintCount;++g){
                const float glintAngle=baseRotation*1.7f+hashf(50u+static_cast<unsigned int>(g))*PI*2.0f;
                const float glintRadius=spread*0.4f*hashf(60u+static_cast<unsigned int>(g));
                const Vec3 glintOffset=tangent*(std::cos(glintAngle)*glintRadius)
                    +bitangent*(std::sin(glintAngle)*glintRadius)+surfaceNormal*0.011f;
                const float glintSize=spread*(0.12f+0.10f*hashf(70u+static_cast<unsigned int>(g)));
                const Quat orientation=quatAxisAngle(surfaceNormal,glintAngle)*tilt;
                drawBox(particle.pos+glintOffset,{glintSize,0.006f,glintSize},orientation,color.r,color.g,color.b,0.38f*t);
            }
        } else {
            drawBox(particle.pos,{size,size,size},particle.life*8.0f,particle.life*4.0f,particle.life*6.0f,color.r,color.g,color.b,(particle.material==ParticleMaterial::Environment?0.82f*t:0.9f));
        }
    }
    if(state.localSettings.particles||state.localSettings.portalWindow)drawDoorDataMosh(state);
    if(codec&&codec->showColliders){
        glDisable(GL_LIGHTING);glDisable(GL_CULL_FACE);glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);glLineWidth(2.0f);
        const float z0=static_cast<float>(state.topology.currentTileIndex)*ROOM_DEPTH;
        for(int i=0;i<state.debug.colliderCount;++i){const auto& c=state.roomColliders[i];drawBox({c.center.x,c.center.y,z0+c.center.z},{c.width,c.height,c.depth},0,0,0,VisualIdentity::ElectricMagenta.r,VisualIdentity::ElectricMagenta.g,VisualIdentity::ElectricMagenta.b);}
        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);glEnable(GL_CULL_FACE);glEnable(GL_LIGHTING);
    }
    if(hudVisible_)drawHud(state);
    if(codec&&codec->open)drawDeveloperCodec(*codec);
    glFlush();
}

void DesktopRenderer::drawDeveloperCodec(const DeveloperCodecState& codec) const{
    glDisable(GL_DEPTH_TEST);glDisable(GL_LIGHTING);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width_,height_,0,-1,1);glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    const auto quad=[](float x,float y,float w,float h,float r,float g,float b,float a){glColor4f(r,g,b,a);glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();};
    const auto text=[&](const std::string& value,float x,float y,float scale,float r,float g,float b,float a){float pen=x;for(char c:value){if(c==' '){pen+=6*scale;continue;}const auto rows=bitmapGlyph(c);for(int row=0;row<7;++row)for(int col=0;col<5;++col)if(rows[row]&(1u<<(4-col)))quad(pen+col*scale,y+row*scale,scale,scale,r,g,b,a);pen+=6*scale;}};
    const float w=std::min(860.0f,static_cast<float>(width_)-32.0f),h=250.0f,x=(width_-w)*0.5f,y=height_-h-22.0f;
    quad(x,y,w,h,0.005f,0.012f,0.016f,0.92f);quad(x,y,w,2,VisualIdentity::ElectricCyan.r,VisualIdentity::ElectricCyan.g,VisualIdentity::ElectricCyan.b,0.90f);
    text("DEVELOPER CODEC  LOCAL / ALLOWLISTED",x+14,y+12,1.25f,0.72f,0.96f,1.0f,0.95f);
    float lineY=y+38.0f;for(int i=0;i<codec.outputCount;++i,lineY+=17.0f)text(codec.output[i],x+14,lineY,1.05f,0.78f,0.90f,0.92f,0.92f);
    quad(x+10,y+h-38,w-20,26,0.01f,0.02f,0.025f,0.96f);text("> "+codec.input+"_",x+18,y+h-31,1.18f,VisualIdentity::AcidChartreuse.r,VisualIdentity::AcidChartreuse.g,VisualIdentity::AcidChartreuse.b,1.0f);
    glMatrixMode(GL_MODELVIEW);glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);glDisable(GL_BLEND);glEnable(GL_DEPTH_TEST);glEnable(GL_LIGHTING);
}
