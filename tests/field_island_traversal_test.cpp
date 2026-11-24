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
