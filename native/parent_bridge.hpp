#pragma once
#include "Game.hpp"
#include "PhoneDisplayLayout.hpp"
#include "developer/DeveloperCodec.hpp"
#include <iomanip>
#include <sstream>
#include <string>
// No replacement state or simulation. This is a thin adapter over original Game.
class ParentBridge {
public:
    ParentBridge() { game.reset(); }
    void input(float x, float z, float look_x, float look_y, bool vacuum, bool sprint,
               bool jump, bool melee, bool shoot, bool camera) {
        game.setTouchControls(x,z,look_x,look_y,vacuum,sprint,jump,melee,shoot,camera);
    }
    void tick() { game.update(1.0f/60.0f); }
    void setCommSignal(int signal) { game.setCommSignal(signal); }
    void setPaused(bool paused) { game.setUiPaused(paused); }
    void menuMoveSelection(int direction) {
        if (direction == 0) return;
        auto& state = game.networkMutableState();
        const auto page = makePhoneMenuPageModel(state);
        const int count = page.selectableCount;
        if (count <= 0) return;
        const int current = std::max(0, std::min(count - 1, state.hud.menuSelection));
        int next = current + (direction > 0 ? 1 : -1);
        if (state.dead) next = std::max(0, std::min(count - 1, next));
        else next = (next + count) % count;
        if (next == current) return;
        state.hud.menuSelection = next;
        const auto layout = makePhoneDisplayMenuLayout(state);
        state.localSettings.menuScroll = phoneDisplayScrollForSelection(layout, next);
        state.cinematic.menuPaintGesture = 0;
        state.cinematic.textInteraction = std::max(state.cinematic.textInteraction, 0.22f);
    }
    bool menuAdjust(int direction) {
        if (direction == 0) return false;
        auto& state = game.networkMutableState();
        const auto page = makePhoneMenuPageModel(state);
        const auto* row = phoneMenuElementForSelection(page, state.hud.menuSelection);
        if (!row) return false;
        const bool adjusted = adjustPhoneMenuSetting(state.localSettings, row->action, direction);
        const bool toggled = !adjusted && togglePhoneMenuSetting(state.localSettings, row->action);
        if (!adjusted && !toggled) return false;
        state.cinematic.menuPaintGesture = direction < 0 ? 2 : 3;
        state.cinematic.textInteraction = 0.65f;
        return true;
    }
    bool menuConfirm() {
        auto& state = game.networkMutableState();
        const auto page = makePhoneMenuPageModel(state);
        const auto* row = phoneMenuElementForSelection(page, state.hud.menuSelection);
        if (!row) return false;
        state.cinematic.menuPaintGesture = 1;
        state.cinematic.textInteraction = 0.42f;
        if (state.dead) {
            if (row->action == PhoneMenuAction::Restart) game.restart();
            else game.prepareStartScreen();
            return true;
        }
        if (state.started && state.uiPaused) {
            if (row->action == PhoneMenuAction::Resume) game.setUiPaused(false);
            else if (row->action == PhoneMenuAction::ExitRun) game.prepareStartScreen();
            else if (row->action == PhoneMenuAction::Settings) pushMenuPage(LocalMenuPage::Settings);
            else if (row->action == PhoneMenuAction::Controls) pushMenuPage(LocalMenuPage::Controls);
            else if (row->action == PhoneMenuAction::Audio) pushMenuPage(LocalMenuPage::Audio);
            else if (row->action == PhoneMenuAction::Graphics) pushMenuPage(LocalMenuPage::Graphics);
            else return menuAdjust(1);
            return true;
        }
        if (!state.started) {
            if (row->action == PhoneMenuAction::Solo) game.restart();
            else if (row->action == PhoneMenuAction::Settings) pushMenuPage(LocalMenuPage::Settings);
            else if (row->action == PhoneMenuAction::Controls) pushMenuPage(LocalMenuPage::Controls);
            else if (row->action == PhoneMenuAction::Audio) pushMenuPage(LocalMenuPage::Audio);
            else if (row->action == PhoneMenuAction::Graphics) pushMenuPage(LocalMenuPage::Graphics);
            else if (row->action == PhoneMenuAction::Back) menuBack();
            else if (row->action == PhoneMenuAction::Defaults) resetControls();
            else if (row->action == PhoneMenuAction::Rebind) {
                state.localSettings.rebindingAction = row->bindingAction;
            } else return menuAdjust(1);
            return true;
        }
        return false;
    }
    bool menuBack() {
        auto& state = game.networkMutableState();
        if (state.dead) return false;
        if (state.localSettings.menuHistoryDepth > 0) {
            const auto entry = state.localSettings.menuHistory[--state.localSettings.menuHistoryDepth];
            state.localSettings.menuPage = entry.page;
            state.localSettings.menuScroll = entry.scroll;
            state.localSettings.rebindingAction = -1;
            state.hud.menuSelection = entry.selection;
            return true;
        }
        if (state.started && state.uiPaused) {
            game.setUiPaused(false);
            return true;
        }
        return false;
    }
    bool upgradeMenuActive() const { return game.state().upgradeMenu.active; }
    void moveUpgradeSelection(int horizontal, int vertical) {
        auto& state = game.networkMutableState();
        if (!state.upgradeMenu.active) return;
        int selection = std::max(0, std::min(5, state.hud.menuSelection));
        int row = selection / 3;
        int column = selection % 3;
        if (horizontal != 0) column = (column + (horizontal > 0 ? 1 : 2)) % 3;
        if (vertical != 0) row = 1 - row;
        const int next = row * 3 + column;
        if (next == state.hud.menuSelection) return;
        state.hud.menuSelection = next;
        state.cinematic.menuPaintGesture = 0;
        state.cinematic.textInteraction = std::max(state.cinematic.textInteraction, 0.22f);
    }
    void setUpgradeSelection(int selection) {
        auto& state = game.networkMutableState();
        if (!state.upgradeMenu.active) return;
        const int next = std::max(0, std::min(5, selection));
        if (next == state.hud.menuSelection) return;
        state.hud.menuSelection = next;
        state.cinematic.menuPaintGesture = 0;
        state.cinematic.textInteraction = std::max(state.cinematic.textInteraction, 0.22f);
    }
    bool activateUpgradeSelection() {
        const auto& state = game.state();
        if (!state.upgradeMenu.active) return false;
        const int selection = std::max(0, std::min(5, state.hud.menuSelection));
        auto& mutableState = game.networkMutableState();
        mutableState.cinematic.menuPaintGesture = 1;
        mutableState.cinematic.textInteraction = 0.42f;
        return selection < 3 ? game.chooseTemporaryUpgrade(selection)
            : game.purchasePermanentUpgrade(selection - 3);
    }
    // Verification fixture delegates setup to the original Game debug scenario.
    void startRallyFixture() { game.debugStartRallyLab(); }
    void startTargetFixture() { game.debugStartRoomInspector(); game.debugToggleRoomInspectorEnemies(); }
    void startTargetInteractionFixture() { game.reset(); }
    void startCityRoomFixture() {
        game.debugStartGeneratedRoomFixture(
            room_environment::representativeInspectorSeed(room_environment::RoomPremise::CityCourtyard), 12);
    }
    void startCoastalRoomFixture() {
        constexpr int roomIndex = 12;
        int seed = room_environment::representativeInspectorSeed(room_environment::RoomPremise::CoastalShore);
        while (!room_environment::matchesInspectorPremise(
            room_environment::roomPlan(seed, roomIndex), room_environment::RoomPremise::CoastalShore)) ++seed;
        game.debugStartGeneratedRoomFixture(seed, roomIndex);
    }
    void startSterileRoomFixture() {
        constexpr int roomIndex = 12;
        int seed = room_environment::representativeInspectorSeed(room_environment::RoomPremise::SterileChamber);
        while (!room_environment::matchesInspectorPremise(
            room_environment::roomPlan(seed, roomIndex), room_environment::RoomPremise::SterileChamber)) ++seed;
        game.debugStartGeneratedRoomFixture(seed, roomIndex);
    }
    void startRuinRoomFixture() {
        constexpr int roomIndex = 12;
        int seed = 1;
        auto plan = room_environment::roomPlan(seed, roomIndex);
        while (plan.setting != room_environment::RoomSetting::Field || plan.composition != 2 || plan.recovery())
            plan = room_environment::roomPlan(++seed, roomIndex);
        game.debugStartGeneratedRoomFixture(seed, roomIndex);
    }
    void startUpgradeFixture() {
        game.debugStartRallyLab();
        auto& state = game.networkMutableState();
        state.upgradeMenu.active = true;
        state.upgradeMenu.presentationTime = 0.0f;
        state.uiPaused = true;
        state.hud.menuSelection = 0;
        state.progression.permanent.tokens = 2;
        state.progression.permanent.levels = {{1, 2, 0}};
        state.progression.run.temporaryLevels = {{2, 1, 3}};
    }
    void startRoomTransitionFixture() {
        game.debugStartRallyLab();
        auto& state = game.networkMutableState();
        state.player.pos = {0.0f, 0.08f, -20.72f};
        state.player.vel = {};
        state.player.grounded = true;
        state.camera.yaw = 0.0f;
        state.camera.pitch = 0.0f;
    }
    void refillTargetInteractionBattery() { game.debugFillBattery(); }
    std::string executeDeveloperCommand(const std::string& command) {
        codec.input = command;
        codec.remember(command);
        codec.write("> " + command);

        const auto parsed = parseDeveloperCodecCommand(command);
        std::ostringstream response;

        const auto invalid = [&]() {
            return parsed.recognizedVerb
                ? std::string("INVALID ARGUMENTS")
                : std::string("UNKNOWN COMMAND");
        };

        switch (parsed.command) {
            case DeveloperCodecCommand::Help:
                response << "help | state | soul spawn | battery full\n"
                         << "enemies on/off/toggle | room next/reroll\n"
                         << "colliders show/hide/toggle | playtest rally/traversal/rooms";
                break;

            case DeveloperCodecCommand::State: {
                const auto& s = game.state();
                response << "room " << s.roomIndex
                         << " seed " << s.roomSeed
                         << "\nsouls " << s.player.souls
                         << " battery " << std::fixed
                         << std::setprecision(1) << s.player.battery
                         << "\nmode "
                         << (s.rallyLab ? "rally" :
                             s.traversalLab ? "traversal" :
                             s.roomInspector ? "rooms" : "game");
                break;
            }

            case DeveloperCodecCommand::SoulSpawn:
                response << (game.debugSpawnStoredSoul()
                    ? "SOUL SPAWNED" : "SOUL WALLET FULL");
                break;

            case DeveloperCodecCommand::BatteryFull:
                game.debugFillBattery();
                response << "BATTERY FULL";
                break;

            case DeveloperCodecCommand::EnemiesOn:
                game.debugSetEnemies(1);
                response << "ENEMIES ON";
                break;

            case DeveloperCodecCommand::EnemiesOff:
                game.debugSetEnemies(0);
                response << "ENEMIES OFF";
                break;

            case DeveloperCodecCommand::EnemiesToggle:
                response << (game.debugSetEnemies(-1)
                    ? "ENEMIES ON" : "ENEMIES OFF");
                break;

            case DeveloperCodecCommand::RoomNext:
                game.debugNextRoom();
                response << "ROOM NEXT";
                break;

            case DeveloperCodecCommand::RoomReroll:
                game.debugRerollRoom();
                response << "ROOM REROLLED";
                break;

            case DeveloperCodecCommand::CollidersShow:
                codec.showColliders = true;
                response << "COLLIDERS SHOWN";
                break;

            case DeveloperCodecCommand::CollidersHide:
                codec.showColliders = false;
                response << "COLLIDERS HIDDEN";
                break;

            case DeveloperCodecCommand::CollidersToggle:
                codec.showColliders = !codec.showColliders;
                response << (codec.showColliders
                    ? "COLLIDERS SHOWN" : "COLLIDERS HIDDEN");
                break;

            case DeveloperCodecCommand::PlaytestRally:
                game.debugStartRallyLab();
                response << "PLAYTEST RALLY";
                break;

            case DeveloperCodecCommand::PlaytestTraversal:
                game.debugStartTraversalLab();
                response << "PLAYTEST TRAVERSAL";
                break;

            case DeveloperCodecCommand::PlaytestRooms:
                game.debugStartRoomInspector();
                response << "PLAYTEST ROOMS";
                break;

            case DeveloperCodecCommand::Lighting:
                response << "UNSUPPORTED COMMAND: lighting";
                break;

            default:
                response << invalid();
                break;
        }

        codec.input.clear();

        const std::string result = response.str();
        std::istringstream lines(result);
        std::string line;

        while (std::getline(lines,line)) {
            codec.write(line);
        }

        return result;
    }

    void setDeveloperConsoleOpen(bool open) {
        codec.open = open;
    }

    bool developerConsoleOpen() const {
        return codec.open;
    }

    bool developerShowColliders() const {
        return codec.showColliders;
    }
    const GameState& state() const { return game.state(); }
private:
    void pushMenuPage(LocalMenuPage page) {
        auto& state = game.networkMutableState();
        auto& settings = state.localSettings;
        if (settings.menuHistoryDepth < LocalSettingsState::MenuHistoryCapacity) {
            settings.menuHistory[settings.menuHistoryDepth++] = {
                settings.menuPage, state.hud.menuSelection, settings.menuScroll};
        } else {
            for (int i = 1; i < LocalSettingsState::MenuHistoryCapacity; ++i)
                settings.menuHistory[i - 1] = settings.menuHistory[i];
            settings.menuHistory[LocalSettingsState::MenuHistoryCapacity - 1] = {
                settings.menuPage, state.hud.menuSelection, settings.menuScroll};
        }
        settings.menuPage = page;
        settings.menuScroll = 0.0f;
        settings.rebindingAction = -1;
        state.hud.menuSelection = 0;
    }
    void resetControls() {
        auto& settings = game.networkMutableState().localSettings;
        settings.keyboardBindings = {{87,83,65,68,340,32,70,81,67,0}};
        settings.mouseLookSensitivity = 1.0f;
        settings.controllerLookSensitivity = 1.15f;
        settings.controllerTriggerSensitivity = 1;
        settings.controllerVibration = 1;
    }
    Game game;
    DeveloperCodecState codec;
};
