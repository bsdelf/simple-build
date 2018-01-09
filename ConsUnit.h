#pragma once

#include <string>
#include <vector>

struct ConsUnit
{
    std::string dir; // output dir
    std::string in;

    std::string out;
    std::string cmd;

#ifdef DEBUG
    std::vector<std::string> deps;
#endif

    ConsUnit(const std::string& workdir, const std::string& infile):
        dir(workdir),
        in(infile)
    {
    }

    std::string Note(bool verbose) const;

    static bool InitC(ConsUnit& unit, const std::string& compiler, const std::string& flags);
    static bool InitCpp(ConsUnit& unit, const std::string& compiler, const std::string& flags);
    static bool InitAsm(ConsUnit& unit, const std::string& compiler, const std::string& flags);
};
