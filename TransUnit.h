#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct TransUnit
{
    std::string in;
    std::string out;
    std::string cmd;
#ifdef DEBUG
    std::vector<std::string> deps;
#endif

    explicit operator bool () const;

    auto Note(bool verbose) const -> std::string;

    static auto Make(std::string file, std::string outdir, const std::unordered_map<std::string, std::string>& args, bool& is_cpp) -> TransUnit;
};
