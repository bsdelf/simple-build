#pragma once

enum class UpdateMode {
    Set = 1,
    Append
};

using ArgMap = std::unordered_map<std::string, std::pair<UpdateMode, std::string>>;

