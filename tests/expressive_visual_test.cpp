#include "HumanVisual.hpp"
#include "HumanModelData.hpp"
#include "VisualIdentity.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    PhoneVisualState previous{};
    for (int frame = 0; frame < 24; ++frame) {
        auto next = makePhoneVisualState(1.0f, 1.0f, 1.0f, frame / 60.0f, false);
        advancePhoneIngestBulge(next, previous, 1.0f / 60.0f);
        previous = next;
    }
    assert(previous.ingestBulge > 0.70f);
    assert(previous.bodyScale.x > 1.05f);
    assert(previous.bodyScale.y > 1.05f);
    assert(previous.bodyScale.z > previous.bodyScale.x);

    for (int frame = 0; frame < 90; ++frame) {
        auto next = makePhoneVisualState(0.0f, 0.0f, 0.0f, (24 + frame) / 60.0f, false);
        advancePhoneIngestBulge(next, previous, 1.0f / 60.0f);
        previous = next;
    }
    assert(std::abs(previous.ingestBulge) < 0.02f);
    assert(std::abs(previous.bodyScale.x - 1.0f) < 0.01f);

    HumanReactionVisual hit{};
    hit.hitAmount = 1.0f;
    hit.hitDirectionLocal = 1.0f;
    const auto expressive = makeHumanVisualPose(0.0f, 1.0f, 0.1f, hit, true);
    assert(expressive.expressiveScale.x > 1.0f);
    assert(expressive.expressiveScale.y < 1.0f);
    assert(expressive.expressiveScale.z > 1.0f);

    const auto searchingReaction = makeHumanReactionVisual(
        0.0f, 0.25f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, true,
        0.0f, 0, 0.8f, 0.9f, 0.35f, 0.0f, 1.0f, -0.6f);
    const auto searching = makeHumanVisualPose(0.0f, 1.0f, 0.73f, searchingReaction, true);
    const auto committedReaction = makeHumanReactionVisual(
        0.0f, 0.8f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, true,
        0.0f, 0, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.6f);
    const auto committed = makeHumanVisualPose(0.0f, 1.0f, 0.73f, committedReaction, true);
    assert(std::abs(searching.torsoRoll) > 0.03f);
    assert(committed.torsoPitch < searching.torsoPitch - 0.04f);
    assert(std::abs(searching.leftArmSwing-searching.rightArmSwing) > 0.08f);

    HumanBodyPresentation physical;
    physical=makeHumanBodyPresentation(0.42f,-0.31f,0.0f,0.0f,true,true,
        {-0.18f,0.04f,0.12f},{0.20f,-0.03f,-0.11f});
    HumanReactionVisual struck=makeHumanReactionVisual(0.4f,1.0f,1.0f,1.0f,0.0f,0.0f,0.0f,true);
    const auto embodied=makeHumanVisualPose(0.0f,1.0f,0.73f,struck,true,physical);
    assert(std::abs(embodied.torsoPitch-physical.pitch())<0.08f);
    assert(std::abs(embodied.torsoRoll-physical.roll())<0.08f);
    assert(embodied.rootBob<0.0f);
    assert(length(physical.leftFootTarget()-Vec3{-0.18f,0.04f,0.12f})<0.016f);
    assert(length(physical.rightFootTarget()-Vec3{0.20f,-0.03f,-0.11f})<0.016f);

    HumanBodyPresentation next=physical;
    next=makeHumanBodyPresentation(-0.20f,0.25f,1.0f,0.5f,false,true);
    const auto midpoint=interpolateHumanBodyPresentation(physical,next,0.5f);
    assert(std::abs(midpoint.pitch()-0.11f)<0.0002f);
    assert(std::abs(midpoint.roll()+0.03f)<0.0002f);
    assert(std::abs(midpoint.leftFootContact()-0.5f)<0.005f&&std::abs(midpoint.rightFootContact()-0.25f)<0.005f);
    assert(!midpoint.fallen()&&midpoint.authoritative());

    HumanModelData model;
    model.frameCount=1;model.unitScale=1.0f;model.minY=0.0f;
    const auto identity=[](float* matrix){for(int i=0;i<16;++i)matrix[i]=i%5==0?1.0f:0.0f;};
    identity(model.bindMatrix);identity(model.bindMatrixInverse);identity(model.rootParentMatrix);
    model.bones.resize(2);model.poses.resize(2);model.vertices.resize(2);
    for(int side=0;side<2;++side){
        auto& bone=model.bones[side];bone.kind=8;bone.side=side==0?-1:1;identity(bone.inverse);
        auto& pose=model.poses[side];pose.quaternion[3]=1.0f;pose.scale[0]=pose.scale[1]=pose.scale[2]=1.0f;
        auto& vertex=model.vertices[side];vertex.position[0]=side==0?-0.10f:0.10f;
        vertex.bones[0]=static_cast<std::uint16_t>(side);vertex.weights[0]=1.0f;
    }
    HumanModelFootTargets footTargets{};footTargets.active=true;
    footTargets.left[0]=-0.24f;footTargets.left[2]=0.08f;footTargets.leftContact=1.0f;
    footTargets.right[0]=0.25f;footTargets.right[2]=-0.06f;footTargets.rightContact=1.0f;
    std::vector<float> corrected;
    model.skin(0.0f,0.0f,0,corrected,{},footTargets);
    assert(std::abs(corrected[0]-footTargets.left[0])<0.0001f&&std::abs(corrected[2]-footTargets.left[2])<0.0001f);
    assert(std::abs(corrected[3]-footTargets.right[0])<0.0001f&&std::abs(corrected[5]-footTargets.right[2])<0.0001f);

    std::puts("EXPRESSIVE_VISUAL_OK phone=INGEST_SPRING enemy=IMPACT_COGNITION_CONTACT");
}
