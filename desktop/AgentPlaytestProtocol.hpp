#pragma once

#include <cmath>
#include <sstream>
#include <string>
#include <unordered_set>

namespace agent_playtest {

inline constexpr int SchemaVersion = 1;
inline constexpr int MaximumStepFrames = 120;
inline constexpr std::size_t MaximumCommandBytes = 1024;

enum class CommandKind : unsigned char { Invalid, Observe, Step, Reset, Quit };

struct StepInput {
    int frames = 1;
    float moveX = 0.0f;
    float moveZ = 0.0f;
    float lookX = 0.0f;
    float lookY = 0.0f;
    bool sprint = false;
    bool vacuum = false;
    bool jump = false;
    bool melee = false;
    bool shoot = false;
    bool camera = false;
};

struct Command {
    CommandKind kind = CommandKind::Invalid;
    StepInput step{};
    std::string error;
};

inline bool parseBool(const std::string& value, bool& result) {
    if (value == "1" || value == "true" || value == "on") { result = true; return true; }
    if (value == "0" || value == "false" || value == "off") { result = false; return true; }
    return false;
}

inline bool parseFiniteFloat(const std::string& value, float minimum, float maximum, float& result) {
    try {
        std::size_t consumed = 0;
        const float parsed = std::stof(value, &consumed);
        if (consumed != value.size() || !std::isfinite(parsed) || parsed < minimum || parsed > maximum)
            return false;
        result = parsed;
        return true;
    } catch (...) { return false; }
}

inline Command parseCommand(const std::string& line) {
    Command result;
    if (line.size() > MaximumCommandBytes) { result.error = "command_too_long"; return result; }
    std::istringstream stream(line);
    std::string verb;
    if (!(stream >> verb)) { result.error = "empty_command"; return result; }
    std::string trailing;
    if (verb == "observe" || verb == "reset" || verb == "quit") {
        if (stream >> trailing) { result.error = "unexpected_argument"; return result; }
        result.kind = verb == "observe" ? CommandKind::Observe
            : (verb == "reset" ? CommandKind::Reset : CommandKind::Quit);
        return result;
    }
    if (verb != "step") { result.error = "unknown_command"; return result; }

    std::unordered_set<std::string> seen;
    std::string token;
    while (stream >> token) {
        const auto equals = token.find('=');
        if (equals == std::string::npos || equals == 0 || equals + 1 >= token.size()) {
            result.error = "expected_key_value"; return result;
        }
        const std::string key = token.substr(0, equals);
        const std::string value = token.substr(equals + 1);
        if (!seen.insert(key).second) { result.error = "duplicate_key:" + key; return result; }
        bool valid = true;
        if (key == "frames") {
            try {
                std::size_t consumed = 0;
                const int parsed = std::stoi(value, &consumed);
                valid = consumed == value.size() && parsed >= 1 && parsed <= MaximumStepFrames;
                if (valid) result.step.frames = parsed;
            } catch (...) { valid = false; }
        } else if (key == "moveX") valid = parseFiniteFloat(value, -1.0f, 1.0f, result.step.moveX);
        else if (key == "moveZ") valid = parseFiniteFloat(value, -1.0f, 1.0f, result.step.moveZ);
        else if (key == "lookX") valid = parseFiniteFloat(value, -400.0f, 400.0f, result.step.lookX);
        else if (key == "lookY") valid = parseFiniteFloat(value, -400.0f, 400.0f, result.step.lookY);
        else if (key == "sprint") valid = parseBool(value, result.step.sprint);
        else if (key == "vacuum") valid = parseBool(value, result.step.vacuum);
        else if (key == "jump") valid = parseBool(value, result.step.jump);
        else if (key == "melee") valid = parseBool(value, result.step.melee);
        else if (key == "shoot") valid = parseBool(value, result.step.shoot);
        else if (key == "camera") valid = parseBool(value, result.step.camera);
        else { result.error = "unknown_key:" + key; return result; }
        if (!valid) { result.error = "invalid_value:" + key; return result; }
    }
    result.kind = CommandKind::Step;
    return result;
}

} // namespace agent_playtest
