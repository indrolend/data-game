#include "Game.hpp"
#include "FieldIsland.hpp"
#include "gameplay/PhoneBody.hpp"

#include <cmath>
#include <cstdio>
#include <memory>

namespace {
constexpr float Dt=1.0f/60.0f;

void neutral(Game& game,int frames=1){for(int i=0;i<frames;++i)game.update(Dt);}
bool near(float a,float b,float tolerance=0.04f){return std::abs(a-b)<=tolerance;}
}

struct FieldIslandTraversalAccess {
    static void populate(Game& game) { game.resetRoom(); }
    static int requested(const Game& game) { return game.activeHumanTarget(); }
    static void queue(Game& game) { game.queueHumanRespawn(game.state().player.pos); }
    static void population(Game& game) { game.updateRoomPopulation(1.0f/60.0f); }
    static void crossing(Game& game) { game.updateRoomTopology(-20.9f,-21.1f); }
    static void television(Game& game) { game.updateSecretTv(1.0f/60.0f); }

    static bool blocked(const Game& game,float x,float z,float radius) {
        return game.isHumanPointBlocked(x,z,radius);
    }
    static std::optional<Vec3> spawn(const Game& game,int index) {
        return game.chooseHumanSpawnPoint(index);
    }
};

int main(){
    auto walk=std::make_unique<Game>();
    auto& walkState=walk->fixtureMutableState();
    walkState.player.pos={field_island::MainHalfWidth-0.35f,0.08f,0.0f};
    walkState.player.vel={6.0f,0.0f,0.0f};
    walkState.player.grounded=true;
    neutral(*walk,45);
    if(walk->state().player.pos.x<=field_island::MainHalfWidth||walk->state().player.grounded||walk->state().player.pos.y>=-0.2f){
        std::fprintf(stderr,"FIELD_ISLAND_FAIL walkoff pos=(%.3f,%.3f,%.3f) grounded=%d\n",walk->state().player.pos.x,walk->state().player.pos.y,walk->state().player.pos.z,walk->state().player.grounded?1:0);return 1;
    }

    auto jump=std::make_unique<Game>();
    auto& jumpState=jump->fixtureMutableState();
    jumpState.player.pos={field_island::MainHalfWidth-0.45f,0.08f,0.0f};
    jumpState.player.vel={6.0f,0.0f,0.0f};
    jump->setTouchControls(0,0,0,0,false,false,true,false,false,false);
    jump->update(Dt);
    jump->setTouchControls(0,0,0,0,false,false,false,false,false,false);
    neutral(*jump,20);
    if(jump->state().player.pos.x<=field_island::MainHalfWidth||jump->state().player.grounded||jump->state().player.pos.y<=0.08f){
        std::fprintf(stderr,"FIELD_ISLAND_FAIL jump-off pos=(%.3f,%.3f,%.3f) grounded=%d\n",jump->state().player.pos.x,jump->state().player.pos.y,jump->state().player.pos.z,jump->state().player.grounded?1:0);return 1;
    }

    auto platform=std::make_unique<Game>();
    auto& platformState=platform->fixtureMutableState();
    platformState.roomColliderCount=1;
    RoomCollider& collider=platformState.roomColliders[0];
    collider.minX=14.0f;collider.maxX=16.0f;collider.minZ=-1.0f;collider.maxZ=1.0f;collider.topY=1.5f;
    platformState.player.pos={15.0f,3.0f,0.0f};platformState.player.vel={};platformState.player.jumpVel=-1.0f;platformState.player.grounded=false;
    neutral(*platform,90);
    if(!platform->state().player.grounded||!near(platform->state().player.pos.y,1.58f)||platform->state().player.supportIdentity.source!=SupportSource::Collider){
        std::fprintf(stderr,"FIELD_ISLAND_FAIL elevated landing y=%.3f grounded=%d support=%d\n",platform->state().player.pos.y,platform->state().player.grounded?1:0,static_cast<int>(platform->state().player.supportIdentity.source));return 1;
    }

    collider.topY = world::RoomWallHeight;
    const auto worldTop = platform->debugWorldSupportAt(15.0f, 0.0f, 0.0f);
    const auto playerTop = platform->debugPlayerSupportAt(15.0f, 0.0f);
    if (!worldTop.available || worldTop.identity.source != SupportSource::Collider ||
        !near(worldTop.height, world::RoomWallHeight + world::GroundY, 0.0001f) ||
        playerTop.identity.source != SupportSource::Collider || playerTop.height >= worldTop.height) {
        std::fprintf(stderr, "FIELD_ISLAND_FAIL world surface must precede player ceiling clearance\n");
        return 1;
    }

    auto recovery=std::make_unique<Game>();
    auto& recoveryState=recovery->fixtureMutableState();
    recoveryState.player.souls=2;
    recoveryState.player.storedSouls[0].id=41;
    recoveryState.captures[0].filled=true;
    recoveryState.captures[0].tokenAwarded=true;
    recoveryState.progression.run.temporaryLevels[0]=2;
    recoveryState.targets[0].alive=true;
    recoveryState.targets[0].armor=1.25f;
    const int roomIndex=recoveryState.roomIndex,roomSeed=recoveryState.roomSeed;
    recoveryState.player.pos={15.0f,field_island::AbyssRecoveryY-0.1f,0.0f};
    recoveryState.player.vel={4.0f,-3.0f,1.0f};recoveryState.player.grounded=false;
    neutral(*recovery);
    const auto& recovered=recovery->state();
    if(recovered.player.abyssRecoveryCount!=1||!recovered.player.grounded||!near(recovered.player.pos.x,0.0f)||!near(recovered.player.pos.y,0.08f)||!near(recovered.player.pos.z,15.5f)||
       recovered.player.souls!=2||recovered.player.storedSouls[0].id!=41||!recovered.captures[0].filled||!recovered.captures[0].tokenAwarded||recovered.progression.run.temporaryLevels[0]!=2||!recovered.targets[0].alive||!near(recovered.targets[0].armor,1.25f)||recovered.roomIndex!=roomIndex||recovered.roomSeed!=roomSeed){
        std::fprintf(stderr,"FIELD_ISLAND_FAIL recovery/preservation count=%d pos=(%.3f,%.3f,%.3f) souls=%d capture=%d room=%d/%d\n",recovered.player.abyssRecoveryCount,recovered.player.pos.x,recovered.player.pos.y,recovered.player.pos.z,recovered.player.souls,recovered.captures[0].filled?1:0,recovered.roomSeed,recovered.roomIndex);return 1;
    }

    auto doorway=std::make_unique<Game>();
    const auto bridgeSupport=doorway->debugPlayerSupportAt(0.0f,-20.0f);
    const auto doorwayFootSupport=doorway->debugPlayerSupportAt(3.1f,-20.9f);
    const auto approachSupport=doorway->debugPlayerSupportAt(3.1f,-field_island::MainHalfDepth);
    const auto abyssSupport=doorway->debugPlayerSupportAt(6.0f,-20.0f);
    if(!bridgeSupport.available||!doorwayFootSupport.available||!approachSupport.available||abyssSupport.available){
        std::fprintf(stderr,"FIELD_ISLAND_FAIL bridge support=%d side support=%d\n",bridgeSupport.available?1:0,abyssSupport.available?1:0);return 1;
    }

    for (const float x : {12.5f, 13.0f, 13.5f}) {
        auto game = std::make_unique<Game>();
        game->debugStartGeneratedRoomFixture(12345, 1);
        auto& state = game->fixtureMutableState();
        state.roomColliderCount = state.slopeSupportCount = state.rockSupportCount = 0;
        for (auto& target : state.targets) target = TargetState{};
        auto& enemy = state.targets[0];
        enemy.alive = true;
        enemy.armor = 2.0f;
        enemy.health = 1.0f;
        enemy.attackCooldown = 999.0f;
        enemy.pos = enemy.walkTarget = {x, world::GroundY, 0.0f};
        auto& particle = state.particles[0];
        particle.material = ParticleMaterial::Environment;
        particle.life = particle.maxLife = 2.0f;
        particle.size = 0.05f;
        particle.pos = {x, 0.12f, 0.0f};
        auto& residue = state.particles[1];
        residue = particle;
        residue.material = ParticleMaterial::SignalResidue;

        bool visible = false;
        for (const auto& surface : field_island::Surfaces) {
            visible |= x <= surface.halfWidth && surface.minZ <= 0 && surface.maxZ >= 0;
        }
        const bool expectedSupport = x < field_island::MainHalfWidth;
        if (game->debugWorldSupportAt(x, 0.0f, 0.0f).available != visible ||
            game->debugWorldSupportAt(x, 0.0f, gameplay::PHONE_BODY.supportRadius).available != expectedSupport ||
            game->debugPlayerSupportAt(x, 0.0f).available != expectedSupport) {
            std::fprintf(stderr, "FIELD_ISLAND_FAIL geometry/support at x=%.2f\n", x);
            return 1;
        }
        neutral(*game, 24);
        const bool settled = (particle.surfaceFlags & 1u) != 0;
        if (settled != expectedSupport ||
            ((residue.surfaceFlags & 1u) != 0) != expectedSupport ||
            !std::isfinite(residue.pos.y) || !std::isfinite(enemy.pos.y) ||
            (expectedSupport ? !near(enemy.pos.y, world::GroundY) : enemy.pos.y >= -0.5f)) {
            std::fprintf(stderr, "FIELD_ISLAND_FAIL enemy or settling particle at x=%.2f\n", x);
            return 1;
        }
        if (!expectedSupport) {
            neutral(*game, 55);
            if (!enemy.alive || !game->debugWorldSupportAt(enemy.pos.x, enemy.pos.z, 0.10f).available ||
                enemy.pos.y < world::GroundY - 0.06f) {
                std::fprintf(stderr, "FIELD_ISLAND_FAIL enemy abyss recovery at x=%.2f\n", x);
                return 1;
            }
        }
    }

    // Initial formations and the scored search use the same ground clearance.
    for (const int seed : {1, 37, 142, 999, 12345, 424242}) {
        auto game = std::make_unique<Game>();
        game->debugStartGeneratedRoomFixture(seed, 1);
        FieldIslandTraversalAccess::populate(*game);
        const auto& state = game->fixtureMutableState();
        for (int i = 0; i < TARGET_COUNT; ++i) {
            const auto& target = state.targets[i];
            if (!target.alive) continue;
            const auto spawn = FieldIslandTraversalAccess::spawn(*game,i);
            if (!spawn ||
                FieldIslandTraversalAccess::blocked(*game,target.pos.x,target.pos.z,0.7f) ||
                FieldIslandTraversalAccess::blocked(*game,spawn->x,spawn->z,0.7f) ||
                !game->debugWorldSupportAt(target.walkTarget.x,target.walkTarget.z,0.1f).available) {
                std::fprintf(stderr,"FINITE_PLACEMENT_FAIL seed=%d slot=%d spawn=%d pos=(%.2f,%.2f) walk=(%.2f,%.2f)\n",
                    seed,i,spawn.has_value(),target.pos.x,target.pos.z,target.walkTarget.x,target.walkTarget.z);
                return 1;
            }
        }
        for (int i = 0; i < state.requiredSouls; ++i) {
            const auto& goal = state.captures[i].pos;
            if (!game->debugWorldSupportAt(goal.x,goal.z,0.5f).available ||
                goal.y <= world::GroundY) {
                std::fprintf(stderr,"FINITE_OBJECTIVE_FAIL seed=%d slot=%d goal=(%.2f,%.2f,%.2f)\n",seed,i,goal.x,goal.y,goal.z);
                return 1;
            }
        }
    }

    auto blockedRoom = std::make_unique<Game>();
    blockedRoom->debugStartGeneratedRoomFixture(12345, 1);
    auto& blockedState = blockedRoom->fixtureMutableState();
    blockedState.roomColliderCount = 1;
    auto& barrier = blockedState.roomColliders[0];
    barrier.minX = -12; barrier.maxX = 12;
    barrier.minZ = -16; barrier.maxZ = 16;
    if (!FieldIslandTraversalAccess::blocked(*blockedRoom,13.5f,0.0f,0.7f) ||
        FieldIslandTraversalAccess::spawn(*blockedRoom,0)) return 1;
    std::printf("FINITE_PLACEMENT_OK seeds=6 unsupported_edge=reject blocked_fallback=reject\n");


    using namespace room_environment;
    auto later = std::make_unique<Game>();
    bool coverage[3][2][4][3]{};
    bool transitionCoverage[2][2]{};
    bool tvCovered = false;
    int cases = 0, maxRequested = 0, maxRetryTicks = 0;
    for (int room : {2, 3, 5, 9, 17, 25}) {
        for (int seed = 1; seed <= 350; ++seed) {
            const auto plan = roomPlan(seed,room);
            const bool floating = hasFiniteGround(plan.boundary);
            if (floating && plan.setting == RoomSetting::Field) {
                bool& covered = coverage[static_cast<int>(plan.scale)][static_cast<int>(plan.condition)]
                    [static_cast<int>(plan.traversalIntent)][plan.composition];
                if (!covered || (!tvCovered && room==3)) {
                    covered = true;
                    later->debugStartGeneratedRoomFixture(seed,room);
                    auto& state = later->fixtureMutableState();
                    state.runRules.requiredSlotStacks = 3;
                    state.runRules.crowdedRoomStacks = 4;
                    FieldIslandTraversalAccess::populate(*later);
                    const int requested = FieldIslandTraversalAccess::requested(*later);
                    int live = 0;
                    for (const auto& target : state.targets) {
                        if (!target.alive) continue;
                        ++live;
                        if (FieldIslandTraversalAccess::blocked(*later,target.pos.x,target.pos.z,0.7f)) return 1;
                    }
                    if (live != requested || !requiredRouteIsTraversable(plan,seed,room) ||
                        later->debugWorldSupportAt(14.0f,0.0f,0).available) return 1;
                    for (int i=0;i<state.requiredSouls;++i) {
                        const auto& goal=state.captures[i].pos;
                        if (!later->debugWorldSupportAt(goal.x,goal.z,0.5f).available) return 1;
                    }
                    state.targets[0] = TargetState{};
                    FieldIslandTraversalAccess::queue(*later);
                    int ticks=0;
                    while (!state.targets[0].alive && ticks < 600) {
                        FieldIslandTraversalAccess::population(*later);
                        ++ticks;
                    }
                    if (!state.targets[0].alive ||
                        FieldIslandTraversalAccess::blocked(*later,state.targets[0].pos.x,state.targets[0].pos.z,0.7f))
                        return 1;
                    maxRequested = std::max(maxRequested,requested);
                    maxRetryTicks = std::max(maxRetryTicks,ticks);
                    ++cases;
                    std::printf("FLOATING_FIELD seed=%d room=%d scale=%s recovery=%d intent=%s composition=%d active=%d goals=%d spawn_ticks=%d\n",
                        seed,room,scaleName(plan.scale),plan.recovery(),traversalIntentName(plan.traversalIntent),
                        plan.composition,live,state.requiredSouls,ticks);
                    if (!tvCovered && room==3) {
                        state.roomClear=true;
                        const auto entrance=state.secretTv.entrancePos;
                        const auto normal=state.secretTv.entranceNormal;
                        state.player.pos=entrance+normal;
                        if (!later->debugPlayerSupportAt(state.player.pos.x,state.player.pos.z).available) return 1;
                        FieldIslandTraversalAccess::television(*later);
                        if (!state.player.inSecretRoom) return 1;
                        state.player.pos.x=43.5f;
                        FieldIslandTraversalAccess::television(*later);
                        if (state.player.inSecretRoom ||
                            !later->debugPlayerSupportAt(state.player.pos.x,state.player.pos.z).available ||
                            !near(state.player.pos.x,entrance.x+normal.x*0.72f)) return 1;
                        tvCovered=true;
                    }
                }
            }
            if (transitionCoverage[1][1]) continue;
            later->debugStartGeneratedRoomFixture(seed,room);
            FieldIslandTraversalAccess::populate(*later);
            auto& state = later->fixtureMutableState();
            state.player.pos = {0,world::GroundY,-21.1f};
            state.player.battery=50;
            if (floating) {
                FieldIslandTraversalAccess::crossing(*later);
                if (state.roomIndex != room || !near(state.player.battery,34)) return 1;
            }
            state.roomClear=true;
            FieldIslandTraversalAccess::crossing(*later);
            if (state.roomIndex != room+1 || !state.upgradeMenu.active ||
                !later->debugPlayerSupportAt(0,-21.1f).available) return 1;
            const bool next = hasFiniteGround(roomPlan(state.roomSeed,state.roomIndex).boundary);
            if (!next ||
                !later->chooseTemporaryUpgrade(0) || state.upgradeMenu.active) return 1;
            if (!transitionCoverage[floating][next]) {
                std::printf("BOUNDARY_TRANSITION seed=%d room=%d %d->%d next=%d battery=%.2f\n",
                    seed,room,floating,next,state.roomSeed,state.player.battery);
            }
            transitionCoverage[floating][next] = true;
        }
    }
    if (!tvCovered || !transitionCoverage[1][1])
        return 1;
    std::printf("LATER_FIELD_OK cases=%d peak_population=%d max_respawn_ticks=%d tv_entry_return=1\n",
        cases,maxRequested,maxRetryTicks);

    // City overlays and local shadow receivers both use this projection.
    const float tileZ = 2.0f * world::RoomDepth;
    int clips = 0, controls = 0;
    bool clipsStayOnGround = true;
    field_island::forEachGroundOverlay({0,0,tileZ},{world::RoomWidth,0.01f,world::RoomDepth},
        tileZ,true,[&](std::size_t,Vec3 center,Vec3 size){
            ++clips;
            for (const float sideX : {-1.0f,1.0f})
                for (const float sideZ : {-1.0f,1.0f}) {
                    clipsStayOnGround &= field_island::hasGroundSupport(
                        center.x+sideX*size.x*0.5f,
                        center.z-tileZ+sideZ*size.z*0.5f,0.0f);
                }
        });
    field_island::forEachGroundOverlay({6,0,tileZ+20},{1,0.01f,1},tileZ,true,
        [&](std::size_t,Vec3,Vec3){++controls;});
    if (clips != 3 || !clipsStayOnGround || controls != 0) {
        std::fprintf(stderr,"FIELD_ISLAND_FAIL ground overlays extend beyond support\n");
        return 1;
    }

    std::printf("FIELD_ISLAND_PASS walkoff jump elevated recovery preservation bridge\n");
    return 0;
}
