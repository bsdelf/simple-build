#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "ArgMap.h"

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

    static auto Make(const std::string& file, const std::string& outdir, const ArgMap& args, bool& is_cpp) -> TransUnit;
};