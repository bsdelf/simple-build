#include "ConsUnit.h"
#include "Tools.h"

#include <iostream>
#include <algorithm>

#include "scx/FileInfo.hpp"
using namespace scx;

// Maybe we can cache mtime to reduce the stat() system calls.
bool ConsUnit::InitC(ConsUnit& unit, const std::string& compiler, const std::string& flags)
{
    const std::string& cmd = compiler + " -MM " + unit.in + " " + flags;
    const std::string& output = DoCmd(cmd);
    const std::string& pattern = "(\\s)+(\\\\)*(\\s)*";
    
    const auto& deps = RegexSplit(output, pattern);
    if (deps.size() < 2) {
        return false;
    }

    unit.out = FileInfo(unit.in).BaseName() + ".o";//deps[0].substr(0, deps[0].size()-1);
    unit.out = unit.dir + unit.out;

    const bool need = FileInfo(unit.out).Exists() ?
        std::any_of(deps.begin(), deps.end(), [&](const auto& dep) { return IsNewer(dep, unit.out); }) :
        true;

    if (need) {
        unit.cmd = compiler + flags + " -o " + unit.out + " -c " + unit.in;
#ifdef DEBUG
        unit.deps.assign(deps.begin() + 1, deps.end());
#endif
    }
    return true;
}

bool ConsUnit::InitCpp(ConsUnit& unit, const std::string& compiler, const std::string& flags)
{
    return InitC(unit, compiler, flags);
}

bool ConsUnit::InitAsm(ConsUnit& unit, const std::string& compiler, const std::string& flags)
{
    unit.out = FileInfo(unit.in).BaseName() + ".o";
    unit.out = unit.dir + unit.out;

    if (!FileInfo(unit.out).Exists() || IsNewer(unit.in, unit.out)) {
        unit.cmd = compiler + " " + flags + " -o " + unit.out + " " + unit.in;
    }

    return true;
}


