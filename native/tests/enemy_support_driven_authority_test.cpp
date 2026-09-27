#include "gameplay/PhysicalEnemyBody.hpp"
#include <cassert>
#include <iostream>

int main() {
    using namespace gameplay;
    PhysicalEnemyBodyState body{};
    PhysicalEnemyBodyInput input{};
    input.dt=1.0f/60.0f;
    input.grounded=true;
    input.supportDrivenLocomotion=true;
    input.desiredVelocity={2.0f,0.0f,0.0f};
    input.actualVelocity={};
    input.leftFootContact=input.rightFootContact=1.0f;
    input.leftFootPosition={0.0f,0.0f,0.0f};
    input.rightFootPosition={0.30f,0.0f,0.0f};
    input.leftFootLoad=1.0f;
    input.rightFootLoad=0.0f;
    auto propelled=updatePhysicalEnemyBody(body,input,0.0f);
    assert(propelled.velocity.x>0.01f);
    assert(propelled.velocity.x<input.desiredVelocity.x);

    // The stance motor is a bounded force, never a desired-velocity assignment.
    input.actualVelocity=propelled.velocity;
    const auto accelerated=updatePhysicalEnemyBody(body,input,0.0f);
    assert(accelerated.velocity.x>propelled.velocity.x);
    assert(accelerated.velocity.x<input.desiredVelocity.x);

    // With no grounded contact, the same motor request produces no propulsion.
    PhysicalEnemyBodyState unsupportedBody{};
    input.actualVelocity={};
    input.leftFootContact=input.rightFootContact=0.0f;
    input.leftFootLoad=input.rightFootLoad=0.0f;
    const auto unsupported=updatePhysicalEnemyBody(unsupportedBody,input,0.0f);
    assert(horizontalLength(unsupported.velocity)<0.0001f);

    // An unreachable planted limb also cannot generate a hidden root force.
    PhysicalEnemyBodyState overextendedBody{};
    input.leftFootContact=1.0f;
    input.leftFootLoad=1.0f;
    input.leftFootPosition={-1.2f,0.0f,0.0f};
    const auto overextended=updatePhysicalEnemyBody(overextendedBody,input,0.0f);
    assert(horizontalLength(overextended.velocity)<0.0001f);
    std::cout<<"ENEMY_SUPPORT_DRIVEN_AUTHORITY_OK stance=BOUNDED_FORCE unsupported=NO_PROPULSION\n";
}
