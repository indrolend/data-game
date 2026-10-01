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

    std::puts("EXPRESSIVE_VISUAL_OK phone=INGEST_SPRING enemy=IMPACT_SQUASH_ARM_REBOUND");
}
