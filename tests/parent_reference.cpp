#include "Game.hpp"
#include "FacetedRock.hpp"
#include "PhoneDisplayLayout.hpp"
#include "room_presentation_snapshot.hpp"
#include "combat_presentation_snapshot.hpp"
#include "../native/phone_presentation_snapshot.hpp"
#include "../native/lighting_presentation_snapshot.hpp"
#include "FieldGrassTexture.hpp"
#include "CitySurfaceTexture.hpp"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>

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

void emitUpgradeSnapshot(const char* stage, const GameState& state) {
    std::cout << "UPGRADE," << stage << ',' << state.upgradeMenu.active << ',' << state.uiPaused << ','
        << state.roomIndex << ',' << state.hud.menuSelection << ',' << state.progression.permanent.tokens;
    for (const int level : state.progression.run.temporaryLevels) std::cout << ',' << level;
    for (const int level : state.progression.permanent.levels) std::cout << ',' << level;
    std::cout << '\n';
}

void emitLightingSnapshot(const char* stage, const GameState& state) {
    const auto snapshot = LightingPresentationSnapshot::from(state);
    const auto& lighting = snapshot.lighting;
    std::cout << "LIGHTING," << stage << ',' << snapshot.roomIndex << ',' << snapshot.setting << ','
        << snapshot.time << ',' << snapshot.daylight << ',' << snapshot.openSky << ','
        << snapshot.background.r << ',' << snapshot.background.g << ','
        << snapshot.background.b << ',' << lighting.ambient.r << ',' << lighting.ambient.g << ','
        << lighting.ambient.b << ',' << lighting.fog.color.r << ',' << lighting.fog.color.g << ','
        << lighting.fog.color.b << ',' << lighting.fog.density << ',' << lighting.sun.enabled << ','
        << lighting.sun.position.x << ',' << lighting.sun.position.y << ',' << lighting.sun.position.z << ','
        << lighting.sun.diffuse.r << ',' << lighting.sun.diffuse.g << ',' << lighting.sun.diffuse.b << ','
        << lighting.fill.enabled << ',' << lighting.fill.position.x << ',' << lighting.fill.position.y << ','
        << lighting.fill.position.z << ',' << lighting.fill.diffuse.r << ',' << lighting.fill.diffuse.g << ','
        << lighting.fill.diffuse.b << ',';
    int localCount = 0;
    for (const auto& light : lighting.localLights) if (light.enabled) ++localCount;
    std::cout << localCount << '\n';
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
        std::isfinite(p.screenLightColor.x)&&std::isfinite(p.screenLightColor.y)&&
        std::isfinite(p.screenLightColor.z)&&std::isfinite(p.screenLightIntensity)&&
        std::isfinite(p.screenLightRadius)&&std::isfinite(p.screenLightForwardOffset)&&
        std::isfinite(p.screenLightPulse)&&
        std::isfinite(p.battery)&&p.battery>=0.0f&&p.battery<=100.0f&&
        p.souls>=0&&p.souls<=PHONE_CAPACITY&&std::isfinite(p.vacuumPower)&&
        std::isfinite(p.capturePulse)&&p.position.x==state.phoneTransform.position.x&&
        p.position.y==state.phoneTransform.position.y&&p.position.z==state.phoneTransform.position.z&&
        p.screenMode==static_cast<int>(state.phoneDisplay.mode)&&p.battery==state.player.battery&&
        p.souls==state.player.souls&&p.firstPerson==state.camera.firstPerson&&
        p.screenLightIntensity==state.phoneDisplay.lighting.intensity&&
        p.screenLightRadius==state.phoneDisplay.lighting.radius&&
        p.screenLightForwardOffset==state.phoneDisplay.lighting.forwardOffset&&
        p.screenLightPulse==state.phoneDisplay.lighting.pulse&&
        p.screenLightColor.x==state.phoneDisplay.lighting.color.x&&
        p.screenLightColor.y==state.phoneDisplay.lighting.color.y&&
        p.screenLightColor.z==state.phoneDisplay.lighting.color.z;
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
        <<p.vacuumPower<<','<<p.capturePulse<<','
        <<p.screenLightColor.x<<','<<p.screenLightColor.y<<','<<p.screenLightColor.z<<','
        <<p.screenLightIntensity<<','<<p.screenLightRadius<<','
        <<p.screenLightForwardOffset<<','<<p.screenLightPulse<<'\n';
}

void phoneMenuSnapshot(const Game& game) {
    const auto& state = game.state();
    const auto layout = makePhoneDisplayMenuLayout(state);
    const int mode = static_cast<int>(state.phoneDisplay.mode);
    const bool active = (mode >= static_cast<int>(PhoneDisplayMode::MainMenu) &&
        mode <= static_cast<int>(PhoneDisplayMode::Graphics)) ||
        mode == static_cast<int>(PhoneDisplayMode::Pause);
    std::cout << "MENULAYOUT," << layout.logicalW << ',' << layout.logicalH << ','
              << layout.rowCount << ',' << layout.selectableCount << ','
              << state.hud.menuSelection << ',' << active << ',' << layout.titlePx << ','
              << layout.titleCenterY << ',' << layout.scrollOffset << ',' << layout.maxScroll << ','
              << layout.content.x << ',' << layout.content.y << ',' << layout.content.w << ','
              << layout.content.h << ',' << layout.title << ',' << layout.navigationHint << '\n';
    for (int i = 0; i < layout.rowCount; ++i) {
        const auto& row = layout.rows[i];
        std::cout << "MENUROW," << i << ',' << static_cast<int>(row.kind) << ','
                  << static_cast<int>(row.action) << ',' << row.selectableIndex << ','
                  << row.bindingAction << ',' << static_cast<int>(row.horizontal) << ','
                  << row.label << ',' << row.value << ',' << row.labelX << ','
                  << row.valueRightX << ',' << row.baselineY << ',' << row.fontPx << ','
                  << row.selectable << ',' << row.visible << ',' << row.peek << ','
                  << row.fixedFooter << ','
                  << (row.selectable && row.selectableIndex == state.hud.menuSelection) << ','
                  << static_cast<int>(phoneMenuEmphasis(row.action)) << ','
                  << phoneMenuVisualAmount(row.action, state.localSettings) << '\n';
    }
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
              << s.vacuum.power << ',' << s.meleePose << ',' << liveBullets(s) << ',' << alive << ','
              << s.camera.verticalFovDegrees << '\n';
    phoneSnapshot(game);
    if (s.frame == 0) phoneMenuSnapshot(game);
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
    const auto p=TargetPresentationSnapshot::from(state,targetIndex);
    std::cout << "TARGETPOSE," << state.frame << ',' << targetIndex << ','
              << p.position.x << ',' << p.position.y << ',' << p.position.z << ','
              << p.velocity.x << ',' << p.velocity.y << ',' << p.velocity.z << ','
              << p.visualYaw << ',' << p.locomotionAmount << ',' << p.locomotionPhase << ','
              << p.attackTimer << ',' << p.attackPhase << ',' << p.attackCommitPhase << ','
              << p.attackEndPhase << ',' << p.attackVariant << ',' << p.attackHit << ','
              << p.attackDirection.x << ',' << p.attackDirection.y << ',' << p.attackDirection.z << ','
              << p.grabbedPlayerId << ',' << p.bodyPitch << ',' << p.bodyRoll << ','
              << p.leftFootContact << ',' << p.rightFootContact << ',' << p.bodyAuthoritative << ','
              << p.pose.rootBob << ',' << p.pose.torsoPitch << ',' << p.pose.torsoRoll << ','
              << p.pose.leftArmSwing << ',' << p.pose.rightArmSwing << ','
              << p.pose.leftLegSwing << ',' << p.pose.rightLegSwing << ','
              << p.pose.hitLean << ',' << p.pose.collapse << ','
              << p.headCenter.x << ',' << p.headCenter.y << ',' << p.headCenter.z << ','
              << p.headRadius << ',' << p.shellCrackAmount << ','
              << p.damageSurfaceColor.r << ',' << p.damageSurfaceColor.g << ','
              << p.damageSurfaceColor.b << '\n';
    std::cout << "TETHER," << state.frame << ',' << targetIndex << ',' << p.tetherVisible << ','
              << p.tetherAnchor.x << ',' << p.tetherAnchor.y << ',' << p.tetherAnchor.z << ','
              << p.tetherDestination.x << ',' << p.tetherDestination.y << ',' << p.tetherDestination.z << ','
              << p.tetherWidth << '\n';
    std::cout << "HIT," << state.frame << ',' << targetIndex << ',' << p.hitAmount << ','
              << p.hitDirectionLocal << ',' << p.pose.hitLean << ',' << p.pose.collapse << '\n';
}

void actionSnapshot(const Game& game) {
    const auto& state=game.state();
    const auto p=PlayerActionPresentationSnapshot::from(state);
    std::cout << "MELEE," << state.frame << ',' << p.sequence << ',' << p.comboIndex << ','
              << p.variant << ',' << p.visualTimer << ',' << p.visualDuration << ','
              << p.normalizedPhase << ',' << p.range << ',' << p.hitRadius << ',' << p.damage << ','
              << p.origin.x << ',' << p.origin.y << ',' << p.origin.z << ','
              << p.direction.x << ',' << p.direction.y << ',' << p.direction.z << ','
              << p.impact.x << ',' << p.impact.y << ',' << p.impact.z << ','
              << p.visualHit << ',' << p.hitMask << ',' << p.locomotionLunge << ','
              << p.airLungePending << ',' << p.airLungeLandingPending << ','
              << p.airLungeTimer << ',' << p.airLungeRotation << ','
              << p.landingRecovery << ',' << p.landingPosePitch << ','
              << p.playerPosition.x << ',' << p.playerPosition.y << ',' << p.playerPosition.z << ','
              << p.playerVelocity.x << ',' << p.playerVelocity.y << ',' << p.playerVelocity.z << '\n';
}
}

int main() {
    Game game;
    game.reset();
    std::cout << std::fixed << std::setprecision(7);
    snapshot(game);
    const auto grassPixels = field_grass_texture::pixels();
    unsigned int grassSum = 0;
    for (const auto pixel : grassPixels) grassSum += pixel;
    std::cout << "GRASSTEX,64,64," << grassPixels.size() << ','
              << static_cast<int>(grassPixels[0]) << ','
              << static_cast<int>(grassPixels[1]) << ','
              << static_cast<int>(grassPixels[2]) << ','
              << static_cast<int>(grassPixels[3072 + 0]) << ','
              << static_cast<int>(grassPixels[3072 + 1]) << ','
              << static_cast<int>(grassPixels[3072 + 2]) << ','
              << static_cast<int>(grassPixels[grassPixels.size() - 3]) << ','
              << static_cast<int>(grassPixels[grassPixels.size() - 2]) << ','
              << static_cast<int>(grassPixels[grassPixels.size() - 1]) << ','
              << grassSum << '\n';
    const int grassTile = game.state().topology.currentTileIndex;
    const float grassTileOffset = static_cast<float>(grassTile) * world::RoomDepth;
    const auto firstBlade = room_environment::grassBlade(
        game.state().roomSeed, game.state().roomIndex, grassTile, 0);
    const auto lastBlade = room_environment::grassBlade(
        game.state().roomSeed, game.state().roomIndex, grassTile,
        room_environment::GrassBladeCountLow - 1);
    std::cout << "GRASSBLADES," << room_environment::GrassBladeCountLow << ','
              << firstBlade.root.x << ',' << firstBlade.root.y << ',' << firstBlade.root.z + grassTileOffset << ','
              << firstBlade.height << ',' << firstBlade.width << ',' << firstBlade.phase << ','
              << lastBlade.root.x << ',' << lastBlade.root.y << ',' << lastBlade.root.z + grassTileOffset << ','
              << lastBlade.height << ',' << lastBlade.width << ',' << lastBlade.phase << '\n';
    room_environment::GrassReactionInputs grassInputs;
    grassInputs.player = game.state().player.pos;
    grassInputs.vacuumOrigin = game.state().phoneTransform.vacuumPullPoint;
    grassInputs.vacuumStrength = game.state().vacuum.fieldStrength;
    grassInputs.shotOrigin = game.state().environmentVisual.latestShotOrigin;
    grassInputs.shotAge = game.state().environmentVisual.latestShotAge;
    const auto grassTipAt = [&](int index) {
        auto blade = room_environment::grassBlade(game.state().roomSeed,
            game.state().roomIndex, grassTile, index);
        blade.root.z += grassTileOffset;
        return room_environment::grassTip(blade, game.state().time, grassInputs);
    };
    const auto firstGrassTip = grassTipAt(0);
    const auto middleGrassTip = grassTipAt(room_environment::GrassBladeCountLow / 2);
    const auto lastGrassTip = grassTipAt(room_environment::GrassBladeCountLow - 1);
    std::cout << "GRASSTIPS,3," << firstGrassTip.x << ',' << firstGrassTip.y << ',' << firstGrassTip.z << ','
              << middleGrassTip.x << ',' << middleGrassTip.y << ',' << middleGrassTip.z << ','
              << lastGrassTip.x << ',' << lastGrassTip.y << ',' << lastGrassTip.z << '\n';
    const auto asphaltPixels = city_surface_texture::pixels();
    unsigned int asphaltSum = 0;
    for (const auto pixel : asphaltPixels) asphaltSum += pixel;
    std::cout << "CITYTEX,32,32," << asphaltPixels.size() << ','
              << static_cast<int>(asphaltPixels[0]) << ','
              << static_cast<int>(asphaltPixels[1]) << ','
              << static_cast<int>(asphaltPixels[2]) << ','
              << static_cast<int>(asphaltPixels[1584]) << ','
              << static_cast<int>(asphaltPixels[1585]) << ','
              << static_cast<int>(asphaltPixels[1586]) << ','
              << static_cast<int>(asphaltPixels[asphaltPixels.size() - 3]) << ','
              << static_cast<int>(asphaltPixels[asphaltPixels.size() - 2]) << ','
              << static_cast<int>(asphaltPixels[asphaltPixels.size() - 1]) << ','
              << asphaltSum << '\n';
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
    Game melee;
    melee.reset();
    actionSnapshot(melee);
    submit(melee,0,0,0,0,false,false,false,true,false,false);
    melee.update(1.0f/60.0f);
    actionSnapshot(melee);
    for(int i=0;i<5;++i){submit(melee,0,0,0,0,false,false,false,false,false,false);melee.update(1.0f/60.0f);}
    actionSnapshot(melee);
    for(int i=0;i<9;++i){submit(melee,0,0,0,0,false,false,false,false,false,false);melee.update(1.0f/60.0f);}
    actionSnapshot(melee);
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

    Game communication;
    communication.reset();
    communication.setCommSignal(3);
    communication.update(1.0f / 60.0f);
    std::cout << "SIGNAL," << communication.state().frame << ','
              << communication.state().player.commSignal << ','
              << communication.state().player.commSignalTimer << '\n';
    for (int i = 0; i < 60; ++i) communication.update(1.0f / 60.0f);
    std::cout << "SIGNAL," << communication.state().frame << ','
              << communication.state().player.commSignal << ','
              << communication.state().player.commSignalTimer << '\n';
    for (int i = 0; i < 90; ++i) communication.update(1.0f / 60.0f);
    std::cout << "SIGNAL," << communication.state().frame << ','
              << communication.state().player.commSignal << ','
              << communication.state().player.commSignalTimer << '\n';

    Game pause;
    pause.reset();
    for (int i = 0; i < 30; ++i) {
        pause.setTouchControls(0, 1, 0, 0, false, false, false, false, false, false);
        pause.update(1.0f / 60.0f);
    }
    std::cout << "PAUSE," << pause.state().frame << ',' << (pause.state().uiPaused ? 1 : 0) << ','
              << pause.state().time << ',' << pause.state().player.pos.x << ','
              << pause.state().player.pos.y << ',' << pause.state().player.pos.z << '\n';
    pause.setUiPaused(true);
    for (int i = 0; i < 60; ++i) {
        pause.setTouchControls(0, 1, 0, 0, false, false, false, false, false, false);
        pause.update(1.0f / 60.0f);
    }
    std::cout << "PAUSE," << pause.state().frame << ',' << (pause.state().uiPaused ? 1 : 0) << ','
              << pause.state().time << ',' << pause.state().player.pos.x << ','
              << pause.state().player.pos.y << ',' << pause.state().player.pos.z << '\n';
    pause.setUiPaused(false);
    for (int i = 0; i < 30; ++i) {
        pause.setTouchControls(0, 1, 0, 0, false, false, false, false, false, false);
        pause.update(1.0f / 60.0f);
    }
    std::cout << "PAUSE," << pause.state().frame << ',' << (pause.state().uiPaused ? 1 : 0) << ','
              << pause.state().time << ',' << pause.state().player.pos.x << ','
              << pause.state().player.pos.y << ',' << pause.state().player.pos.z << '\n';

    Game room;
    room.reset();
    const auto roomSnapshot = RoomPresentationSnapshot::from(
        room.state(), room.state().topology.currentTileIndex);
    std::cout << "ROOM," << roomSnapshot.roomSeed << ',' << roomSnapshot.roomIndex << ','
              << roomSnapshot.tileIndex << ',' << roomSnapshot.currentTileIndex << ','
              << roomSnapshot.setting << ',' << roomSnapshot.form << ',' << roomSnapshot.scale << ','
              << roomSnapshot.condition << ',' << roomSnapshot.traversalIntent << ','
              << (roomSnapshot.roomClear ? 1 : 0) << ',' << (roomSnapshot.advancing ? 1 : 0) << ','
              << (roomSnapshot.grass ? 1 : 0) << ',' << (roomSnapshot.sidewalks ? 1 : 0) << ','
              << roomSnapshot.grassAmount << ',' << roomSnapshot.structures.size() << ','
              << roomSnapshot.objects.size() << '\n';
    for (const auto& structure : roomSnapshot.structures) {
        std::cout << "ROOMSTRUCT," << structure.id << ',' << static_cast<int>(structure.kind) << ','
                  << structure.center.x << ',' << structure.center.y << ',' << structure.center.z << ','
                  << structure.size.x << ',' << structure.size.y << ',' << structure.size.z << ','
                  << structure.material.baseColor.r << ',' << structure.material.baseColor.g << ','
                  << structure.material.baseColor.b << ',' << static_cast<int>(structure.material.shading) << ','
                  << structure.material.opacity << ',' << (structure.material.fog ? 1 : 0) << ','
                  << static_cast<int>(structure.material.texture) << ',' << structure.material.textureWorldScale << ','
                  << structure.material.specular.r << ',' << structure.material.specular.g << ','
                  << structure.material.specular.b << ',' << structure.material.shininess << ','
                  << structure.material.emission.r << ',' << structure.material.emission.g << ','
                  << structure.material.emission.b << '\n';
    }
    auto coastalRoom = std::make_unique<Game>();
    int coastalSeed = room_environment::representativeInspectorSeed(room_environment::RoomPremise::CoastalShore);
    while (!room_environment::matchesInspectorPremise(
        room_environment::roomPlan(coastalSeed, 12), room_environment::RoomPremise::CoastalShore)) ++coastalSeed;
    coastalRoom->debugStartGeneratedRoomFixture(coastalSeed, 12);
    const auto coastalSnapshot = RoomPresentationSnapshot::from(
        coastalRoom->state(), coastalRoom->state().topology.currentTileIndex);
    std::cout << "COASTALWATER," << (coastalSnapshot.coastalWater ? 1 : 0) << ','
              << coastalSnapshot.presentationTime << ',' << coastal_water::ShoreX << ','
              << coastal_water::FarX << ',' << coastal_water::MinZ << ',' << coastal_water::MaxZ << ','
              << coastalSnapshot.waterSourcePosition.x << ',' << coastalSnapshot.waterSourcePosition.y << ','
              << coastalSnapshot.waterSourcePosition.z << ',' << coastalSnapshot.waterSourceSpeed << ','
              << (coastalSnapshot.waterSourceActive ? 1 : 0) << ','
              << static_cast<float>(coastalSnapshot.tileIndex) * world::RoomDepth << '\n';
    for (const auto& object : roomSnapshot.objects) {
        std::cout << "ROOMOBJ," << object.id << ',' << object.tileIndex << ',' << object.propIndex << ','
                  << object.partIndex << ',' << static_cast<int>(object.primitive) << ','
                  << static_cast<int>(object.role) << ',' << static_cast<int>(object.geometry) << ','
                  << static_cast<int>(object.part) << ',' << object.center.x << ',' << object.center.y << ','
                  << object.center.z << ',' << object.size.x << ',' << object.size.y << ',' << object.size.z << ','
                  << object.yaw << ',' << static_cast<int>(object.surface) << ',' << (object.physical ? 1 : 0) << '\n';
        if (object.geometry == computed_scene::GeometryKind::FacetedMesh) {
            const auto plan = room_environment::roomPlan(roomSnapshot.roomSeed, roomSnapshot.roomIndex);
            const auto prop = room_environment::environmentProp(plan, roomSnapshot.roomSeed,
                roomSnapshot.roomIndex, object.propIndex);
            const auto mesh = faceted_rock::makeVisualMesh(prop, roomSnapshot.roomSeed,
                roomSnapshot.roomIndex, object.propIndex,
                static_cast<float>(object.tileIndex) * world::RoomDepth);
            for (int vertex = 0; vertex < mesh.vertexCount; ++vertex) {
                const int offset = vertex * 3;
                std::cout << "ROCKVERTEX," << object.id << ',' << vertex << ','
                    << mesh.positions[offset] - object.center.x << ','
                    << mesh.positions[offset + 1] - object.center.y << ','
                    << mesh.positions[offset + 2] - object.center.z << ','
                    << mesh.normals[offset] << ',' << mesh.normals[offset + 1] << ','
                    << mesh.normals[offset + 2] << '\n';
            }
        }
    }
    Game cityRoom;
    cityRoom.debugStartGeneratedRoomFixture(
        room_environment::representativeInspectorSeed(room_environment::RoomPremise::CityCourtyard), 12);
    const auto citySnapshot = RoomPresentationSnapshot::from(
        cityRoom.state(), cityRoom.state().topology.currentTileIndex);
    for (const auto& structure : citySnapshot.structures) {
        if (structure.kind == RoomStructureKind::CityStreetFixture ||
            structure.kind == RoomStructureKind::CityServiceFixture) {
            const char* category = structure.kind == RoomStructureKind::CityStreetFixture
                ? "CITYLAMP" : "CITYSERVICE";
            std::cout << category << ',' << structure.id << ',' << structure.center.x << ','
                      << structure.center.y << ',' << structure.center.z << ','
                      << structure.size.x << ',' << structure.size.y << ',' << structure.size.z << ','
                      << structure.material.baseColor.r << ',' << structure.material.baseColor.g << ','
                      << structure.material.baseColor.b << ',' << static_cast<int>(structure.material.shading)
                      << ',' << structure.material.fog << '\n';
            continue;
        }
        if (structure.kind == RoomStructureKind::CitySkylineWindow) {
            std::cout << "CITYWINDOW," << structure.id << ',' << structure.center.x << ','
                      << structure.center.y << ',' << structure.center.z << ','
                      << structure.size.x << ',' << structure.size.y << ',' << structure.size.z << ','
                      << structure.material.baseColor.r << ',' << structure.material.baseColor.g << ','
                      << structure.material.baseColor.b << ',' << structure.material.fog << '\n';
            continue;
        }
        if (structure.kind == RoomStructureKind::CitySkylineMass) {
            std::cout << "CITYSKYLINE," << structure.id << ',' << structure.center.x << ','
                      << structure.center.y << ',' << structure.center.z << ','
                      << structure.size.x << ',' << structure.size.y << ',' << structure.size.z << ','
                      << structure.material.baseColor.r << ',' << structure.material.baseColor.g << ','
                      << structure.material.baseColor.b << '\n';
            continue;
        }
        if (structure.kind == RoomStructureKind::CityBuildingMass) {
            std::cout << "CITYMASS," << structure.id << ',' << structure.center.x << ','
                      << structure.center.y << ',' << structure.center.z << ','
                      << structure.size.x << ',' << structure.size.y << ',' << structure.size.z << ','
                      << structure.material.baseColor.r << ',' << structure.material.baseColor.g << ','
                      << structure.material.baseColor.b << '\n';
            continue;
        }
        if (structure.kind != RoomStructureKind::CityStreet) continue;
        std::cout << "CITYSTRUCT," << structure.id << ',' << structure.center.x << ','
                  << structure.center.y << ',' << structure.center.z << ','
                  << structure.size.x << ',' << structure.size.y << ',' << structure.size.z << ','
                  << structure.material.baseColor.r << ',' << structure.material.baseColor.g << ','
                  << structure.material.baseColor.b << '\n';
    }
    for (const auto& module : citySnapshot.cityFacadeModules) {
        std::cout << "CITYFACADE," << module.id << ',' << module.model << ','
                  << module.origin.x << ',' << module.origin.y << ',' << module.origin.z << ','
                  << module.scale.x << ',' << module.scale.y << ',' << module.scale.z << ','
                  << module.yaw << '\n';
    }
    const auto cityPlan = room_environment::roomPlan(citySnapshot.roomSeed, citySnapshot.roomIndex);
    for (const auto& object : citySnapshot.objects) {
        if (object.primitive != room_environment::EnvironmentPrimitive::House) continue;
        const auto prop = room_environment::environmentProp(cityPlan, citySnapshot.roomSeed,
            citySnapshot.roomIndex, object.propIndex);
        const float variation = 0.92f + 0.045f * static_cast<float>((object.partIndex + prop.variant) % 3);
        VisualColor color{};
        if (object.surface == 0) color = {0.40f * variation, 0.47f * variation, 0.50f * variation};
        else if (object.surface == 3) color = {0.035f, 0.045f, 0.048f};
        else if (object.surface == 1) color = {0.30f * variation, 0.37f * variation, 0.41f * variation};
        else color = {0.26f * variation, 0.32f * variation, 0.36f * variation};
        const char* texture = object.surface == 0
            ? (cityPlan.setting == room_environment::RoomSetting::City ? "metal" : "stone") : "none";
        std::cout << "HOUSEPART," << object.id << ',' << object.propIndex << ',' << object.partIndex << ','
                  << static_cast<int>(object.surface) << ',' << object.center.x << ',' << object.center.y << ','
                  << object.center.z << ',' << object.size.x << ',' << object.size.y << ',' << object.size.z << ','
                  << object.yaw << ',' << object.physical << ',' << color.r << ',' << color.g << ',' << color.b
                  << ',' << texture << '\n';
    }
    Game sterileRoom;
    int sterileSeed = room_environment::representativeInspectorSeed(room_environment::RoomPremise::SterileChamber);
    while (!room_environment::matchesInspectorPremise(
        room_environment::roomPlan(sterileSeed, 12), room_environment::RoomPremise::SterileChamber)) ++sterileSeed;
    sterileRoom.debugStartGeneratedRoomFixture(sterileSeed, 12);
    const auto sterileSnapshot = RoomPresentationSnapshot::from(
        sterileRoom.state(), sterileRoom.state().topology.currentTileIndex);
    for (const auto& object : sterileSnapshot.objects) {
        if (object.primitive != room_environment::EnvironmentPrimitive::MarkerPillar) continue;
        const VisualColor color = object.surface == 0
            ? VisualColor{0.70f, 0.76f, 0.78f} : VisualColor{0.72f, 0.90f, 0.94f};
        std::cout << "MARKERPART," << object.id << ',' << object.partIndex << ',' << static_cast<int>(object.surface) << ','
                  << object.center.x << ',' << object.center.y << ',' << object.center.z << ','
                  << object.size.x << ',' << object.size.y << ',' << object.size.z << ','
                  << (object.physical ? 1 : 0) << ',' << color.r << ',' << color.g << ',' << color.b << ','
                  << (object.surface == 0 ? "metal" : "none") << '\n';
    }
    Game ruinRoom;
    int ruinSeed = 1;
    auto ruinPlan = room_environment::roomPlan(ruinSeed, 12);
    while (ruinPlan.setting != room_environment::RoomSetting::Field || ruinPlan.composition != 2 || ruinPlan.recovery())
        ruinPlan = room_environment::roomPlan(++ruinSeed, 12);
    ruinRoom.debugStartGeneratedRoomFixture(ruinSeed, 12);
    const auto ruinSnapshot = RoomPresentationSnapshot::from(
        ruinRoom.state(), ruinRoom.state().topology.currentTileIndex);
    const auto selectedRuinPlan = room_environment::roomPlan(ruinSnapshot.roomSeed, ruinSnapshot.roomIndex);
    for (const auto& object : ruinSnapshot.objects) {
        if (object.primitive != room_environment::EnvironmentPrimitive::Ruin) continue;
        const auto prop = room_environment::environmentProp(selectedRuinPlan,
            ruinSnapshot.roomSeed, ruinSnapshot.roomIndex, object.propIndex);
        const float weather = 0.88f + 0.055f * static_cast<float>((object.partIndex + prop.variant) % 3);
        const VisualColor color = object.surface == 0
            ? VisualColor{0.84f * weather, 0.82f * weather, 0.72f * weather}
            : VisualColor{0.70f * weather, 0.70f * weather, 0.66f * weather};
        const float phase = std::fmod((prop.variant + object.partIndex) * 0.137f, 1.0f);
        const Vec3 scarCenter = object.center + Vec3{0, -object.size.y * 0.39f, -object.size.z * 0.505f};
        const Vec3 scarSize{object.size.x * 0.74f, 0.045f, 0.024f};
        std::cout << "RUINPART," << object.id << ',' << object.partIndex << ',' << static_cast<int>(object.surface) << ','
                  << object.center.x << ',' << object.center.y << ',' << object.center.z << ','
                  << object.size.x << ',' << object.size.y << ',' << object.size.z << ','
                  << (object.physical ? 1 : 0) << ',' << color.r << ',' << color.g << ',' << color.b
                  << ",stone," << phase << ',' << (object.size.y > 0.42f ? 1 : 0) << ','
                  << scarCenter.x << ',' << scarCenter.y << ',' << scarCenter.z << ','
                  << scarSize.x << ',' << scarSize.y << ',' << scarSize.z << '\n';
    }
    Game upgradeGame;
    upgradeGame.debugStartRallyLab();
    auto& upgradeState = upgradeGame.networkMutableState();
    upgradeState.upgradeMenu.active = true;
    upgradeState.uiPaused = true;
    upgradeState.progression.permanent.tokens = 2;
    upgradeState.progression.permanent.levels = {{1, 2, 0}};
    upgradeState.progression.run.temporaryLevels = {{2, 1, 3}};
    emitUpgradeSnapshot("initial", upgradeGame.state());
    const auto moveUpgrade = [&](int horizontal, int vertical) {
        auto& state = upgradeGame.networkMutableState();
        int selection = std::max(0, std::min(5, state.hud.menuSelection));
        int row = selection / 3, column = selection % 3;
        if (horizontal != 0) column = (column + (horizontal > 0 ? 1 : 2)) % 3;
        if (vertical != 0) row = 1 - row;
        state.hud.menuSelection = row * 3 + column;
    };
    moveUpgrade(1, 0); moveUpgrade(0, 1);
    emitUpgradeSnapshot("grid", upgradeGame.state());
    upgradeGame.purchasePermanentUpgrade(upgradeGame.state().hud.menuSelection - 3);
    emitUpgradeSnapshot("purchase", upgradeGame.state());
    upgradeGame.networkMutableState().hud.menuSelection = 1;
    upgradeGame.chooseTemporaryUpgrade(1);
    emitUpgradeSnapshot("run", upgradeGame.state());

    Game roomCrossing;
    roomCrossing.debugStartRallyLab();
    auto& crossingState = roomCrossing.networkMutableState();
    crossingState.player.pos = {0.0f, 0.08f, -20.72f};
    crossingState.player.vel = {};
    crossingState.player.grounded = true;
    crossingState.camera.yaw = 0.0f;
    crossingState.camera.pitch = 0.0f;
    for (int frame = 0; frame < 120 && !roomCrossing.state().upgradeMenu.active; ++frame) {
        submit(roomCrossing, 0, 1, 0, 0, false, false, false, false, false, false);
        roomCrossing.update(1.0f / 60.0f);
    }
    const auto& crossed = roomCrossing.state();
    std::cout << "ROOMCROSS," << crossed.frame << ',' << crossed.roomIndex << ','
        << crossed.player.pos.x << ',' << crossed.player.pos.y << ',' << crossed.player.pos.z << ','
        << crossed.upgradeMenu.active << ',' << crossed.uiPaused << ',' << crossed.player.battery << ','
        << crossed.player.souls << '\n';
    auto lightingGame = std::make_unique<Game>();
    lightingGame->reset();
    emitLightingSnapshot("initial", lightingGame->state());
    return 0;
}
