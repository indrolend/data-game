#include "Game.hpp"
#include "../native/phone_presentation_snapshot.hpp"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>

namespace {
void submit(Game& game, float x, float z, float lookX, float lookY,
            bool vacuum, bool sprint, bool jump, bool melee, bool shoot, bool camera) {
    game.setTouchControls(x, z, lookX, lookY, vacuum, sprint, jump, melee, shoot, camera);
}

int liveBullets(const GameState& state) {
    int count = 0;
    for (const auto& bullet : state.bullets) if (bullet.alive) ++count;
    return count;
}

bool validPhoneSnapshot(const GameState& state, const PhonePresentationSnapshot& p) {
    const auto finite3=[](const Vec3& v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);};
    const auto finite4=[](const Quat& q){return std::isfinite(q.x)&&std::isfinite(q.y)&&std::isfinite(q.z)&&std::isfinite(q.w);};
    const float qLengthSq=p.orientation.x*p.orientation.x+p.orientation.y*p.orientation.y+
        p.orientation.z*p.orientation.z+p.orientation.w*p.orientation.w;
    return finite3(p.position)&&finite4(p.orientation)&&std::abs(qLengthSq-1.0f)<=0.0001f&&
        p.bodySize.x>0.0f&&p.bodySize.y>0.0f&&p.bodySize.z>0.0f&&
        p.screenSize.x>0.0f&&p.screenSize.y>0.0f&&p.screenSize.x<=p.bodySize.x&&p.screenSize.y<=p.bodySize.y&&
        finite3(p.bodyScale)&&p.bodyScale.x>0.0f&&p.bodyScale.y>0.0f&&p.bodyScale.z>0.0f&&
        finite3(p.screenCenterOffset)&&finite3(p.screenTint)&&
        std::isfinite(p.screenBrightness)&&p.screenBrightness>=0.0f&&p.screenBrightness<=1.0f&&
        std::isfinite(p.contentOpacity)&&p.contentOpacity>=0.0f&&p.contentOpacity<=1.0f&&
        std::isfinite(p.screenGlow)&&std::isfinite(p.emissionStrength)&&
        std::isfinite(p.battery)&&p.battery>=0.0f&&p.battery<=100.0f&&
        p.souls>=0&&p.souls<=PHONE_CAPACITY&&std::isfinite(p.vacuumPower)&&
        std::isfinite(p.capturePulse)&&p.position.x==state.phoneTransform.position.x&&
        p.position.y==state.phoneTransform.position.y&&p.position.z==state.phoneTransform.position.z&&
        p.screenMode==static_cast<int>(state.phoneDisplay.mode)&&p.battery==state.player.battery&&
        p.souls==state.player.souls&&p.firstPerson==state.camera.firstPerson;
}

void phoneSnapshot(const Game& game) {
    const auto& state=game.state();
    const auto p=PhonePresentationSnapshot::from(state);
    if(!validPhoneSnapshot(state,p)){
        std::cerr<<"FAIL: invalid native phone presentation snapshot at frame "<<state.frame<<'\n';
        std::exit(2);
    }
    std::cout<<"PHONE,"<<state.frame<<','
        <<p.position.x<<','<<p.position.y<<','<<p.position.z<<','
        <<p.orientation.x<<','<<p.orientation.y<<','<<p.orientation.z<<','<<p.orientation.w<<','
        <<p.bodySize.x<<','<<p.bodySize.y<<','<<p.bodySize.z<<','
        <<p.bodyScale.x<<','<<p.bodyScale.y<<','<<p.bodyScale.z<<','
        <<p.screenCenterOffset.x<<','<<p.screenCenterOffset.y<<','<<p.screenCenterOffset.z<<','
        <<p.screenSize.x<<','<<p.screenSize.y<<','<<p.screenSize.z<<','
        <<p.screenMode<<','<<p.bodyVisible<<','<<p.screenVisible<<','<<p.firstPerson<<','
        <<p.screenBrightness<<','<<p.contentOpacity<<','
        <<p.screenTint.x<<','<<p.screenTint.y<<','<<p.screenTint.z<<','
        <<p.screenGlow<<','<<p.emissionStrength<<','<<p.interactive<<','
        <<p.battery<<','<<p.souls<<','<<p.lowBattery<<','<<p.vacuumActive<<','
        <<p.vacuumPower<<','<<p.capturePulse<<'\n';
}

void snapshot(const Game& game) {
    const auto& s = game.state();
    int alive = 0;
    for (const auto& target : s.targets) if (target.alive) ++alive;
    std::cout << "PARITY," << s.frame << ','
              << s.player.pos.x << ',' << s.player.pos.y << ',' << s.player.pos.z << ','
              << s.camera.pos.x << ',' << s.camera.pos.y << ',' << s.camera.pos.z << ','
              << s.camera.lookTarget.x << ',' << s.camera.lookTarget.y << ',' << s.camera.lookTarget.z << ','
              << s.player.battery << ',' << s.player.souls << ',' << (s.camera.firstPerson ? 1 : 0) << ','
              << (s.player.grounded ? 1 : 0) << ',' << (s.vacuum.active ? 1 : 0) << ','
              << s.vacuum.power << ',' << s.meleePose << ',' << liveBullets(s) << ',' << alive << '\n';
    phoneSnapshot(game);
}

int nearestLiveTarget(const GameState& state) {
    int nearest = -1;
    float bestDistanceSq = 1e9f;
    for (int i = 0; i < TARGET_COUNT; ++i) if (state.targets[i].alive) {
        const float dx = state.targets[i].pos.x - state.player.pos.x;
        const float dz = state.targets[i].pos.z - state.player.pos.z;
        const float distanceSq = dx * dx + dz * dz;
        if (distanceSq < bestDistanceSq) { bestDistanceSq = distanceSq; nearest = i; }
    }
    return nearest;
}

void interactionSnapshot(const Game& game, int targetIndex) {
    const auto& state = game.state();
    const auto& target = state.targets[targetIndex];
    int alive = 0;
    for (const auto& candidate : state.targets) if (candidate.alive) ++alive;
    std::cout << "INTERACT," << state.frame << ',' << targetIndex << ','
              << target.pos.x << ',' << target.pos.y << ',' << target.pos.z << ','
              << target.armor << ',' << (target.slurpable ? 1 : 0) << ','
              << target.ingestProgress << ',' << (target.alive ? 1 : 0) << ','
              << alive << ',' << state.player.souls << ',' << state.player.battery << '\n';
}
}

int main() {
    Game game;
    game.reset();
    std::cout << std::fixed << std::setprecision(7);
    snapshot(game);
    for (int tick = 0; tick < 240; ++tick) {
        const bool forward = tick < 120;
        const bool sprint = tick >= 60 && tick < 90;
        const bool jump = tick == 90;
        const bool camera = tick == 60;
        const bool vacuum = tick >= 120 && tick < 150;
        const bool melee = tick == 150;
        const bool shoot = tick == 180;
        const float lookX = tick >= 30 && tick < 45 ? 1.5f : 0.0f;
        const float lookY = tick >= 30 && tick < 45 ? -0.5f : 0.0f;
        submit(game, 0.0f, forward ? 1.0f : 0.0f, lookX, lookY,
               vacuum, sprint, jump, melee, shoot, camera);
        game.update(1.0f / 60.0f);
        if ((tick + 1) % 30 == 0) snapshot(game);
    }
    Game rally;
    rally.reset();
    rally.debugStartRallyLab();
    std::cout << "ACTION,fixture," << rally.state().frame << ',' << rally.state().player.souls << ',' << liveBullets(rally.state()) << '\n';
    rally.setTouchControls(0, 0, 0, 0, false, false, false, false, true, false);
    rally.update(1.0f / 60.0f);
    std::cout << "ACTION,shot," << rally.state().frame << ',' << rally.state().player.souls << ',' << liveBullets(rally.state()) << '\n';
    for (int i = 0; i < 8; ++i) {
        rally.setTouchControls(0, 0, 0, 0, false, false, false, false, false, false);
        rally.update(1.0f / 60.0f);
    }
    std::cout << "ACTION,projectile," << rally.state().frame << ',' << rally.state().player.souls << ',' << liveBullets(rally.state()) << '\n';
    Game targets;
    targets.reset();
    targets.debugStartRoomInspector();
    targets.debugToggleRoomInspectorEnemies();
    int liveTargetCount = 0;
    int firstLiveTarget = -1;
    for (int i = 0; i < TARGET_COUNT; ++i) if (targets.state().targets[i].alive) {
        ++liveTargetCount;
        if (firstLiveTarget < 0) firstLiveTarget = i;
    }
    const Vec3 firstTarget = firstLiveTarget >= 0 ? targets.state().targets[firstLiveTarget].pos : Vec3{};
    std::cout << "TARGET," << targets.state().frame << ',' << liveTargetCount << ',' << firstLiveTarget << ','
              << firstTarget.x << ',' << firstTarget.y << ',' << firstTarget.z << '\n';
    for (int i = 0; i < 120; ++i) {
        targets.setTouchControls(0, 0, 0, 0, false, false, false, false, false, false);
        targets.update(1.0f / 60.0f);
    }
    liveTargetCount = 0;
    for (int i = 0; i < TARGET_COUNT; ++i) if (targets.state().targets[i].alive) ++liveTargetCount;
    const Vec3 movedTarget = firstLiveTarget >= 0 ? targets.state().targets[firstLiveTarget].pos : Vec3{};
    std::cout << "TARGET," << targets.state().frame << ',' << liveTargetCount << ',' << firstLiveTarget << ','
              << movedTarget.x << ',' << movedTarget.y << ',' << movedTarget.z << '\n';

    Game interaction;
    interaction.reset();
    const int interactionTarget = nearestLiveTarget(interaction.state());
    interactionSnapshot(interaction, interactionTarget);
    for (int step = 0; step < 1050; ++step) {
        if (step == 960) interaction.debugFillBattery();
        const bool melee = step > 300 && step < 1000 && step % 50 == 0;
        const bool vacuum = step > 960;
        const float lookX = step == 960 ? 120.0f : 0.0f;
        submit(interaction, 0, 0, lookX, 0, vacuum, false, false, melee, false, false);
        interaction.update(1.0f / 60.0f);
        if (interaction.state().frame == 600 || interaction.state().frame == 900 ||
            interaction.state().frame == 960 || interaction.state().frame == 1020 ||
            interaction.state().frame == 1050) interactionSnapshot(interaction, interactionTarget);
    }
    return 0;
}
