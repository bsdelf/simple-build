#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct TransUnit
{
    std::string srcfile;
    std::string objfile;
    std::string command;
#ifdef DEBUG
    std::vector<std::string> depfiles;
#endif

    explicit operator bool () const;

    auto Note(bool verbose) const -> std::string;

    static auto Make(const std::string& file, const std::string& outdir, const std::unordered_map<std::string, std::string>& args, bool& is_cpp) -> TransUnit;
};
