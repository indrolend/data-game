#include "HumanVisual.hpp"
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
    physical=makeHumanBodyPresentation(0.42f,-0.31f,0.0f,0.0f,true,true);
    HumanReactionVisual struck=makeHumanReactionVisual(0.4f,1.0f,1.0f,1.0f,0.0f,0.0f,0.0f,true);
    const auto embodied=makeHumanVisualPose(0.0f,1.0f,0.73f,struck,true,physical);
    assert(std::abs(embodied.torsoPitch-physical.pitch())<0.08f);
    assert(std::abs(embodied.torsoRoll-physical.roll())<0.08f);
    assert(embodied.rootBob<0.0f);

    HumanBodyPresentation next=physical;
    next=makeHumanBodyPresentation(-0.20f,0.25f,1.0f,0.5f,false,true);
    const auto midpoint=interpolateHumanBodyPresentation(physical,next,0.5f);
    assert(std::abs(midpoint.pitch()-0.11f)<0.0002f);
    assert(std::abs(midpoint.roll()+0.03f)<0.0002f);
    assert(std::abs(midpoint.leftFootContact()-0.5f)<0.005f&&std::abs(midpoint.rightFootContact()-0.25f)<0.005f);
    assert(!midpoint.fallen()&&midpoint.authoritative());

    std::puts("EXPRESSIVE_VISUAL_OK phone=INGEST_SPRING enemy=IMPACT_COGNITION_CONTACT");
}
