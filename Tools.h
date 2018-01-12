#pragma once

#include <string>
#include <vector>

auto DoCmd(const std::string& cmd) -> std::string;

auto RegexSplit(const std::string& str, const std::string& pattern) -> std::vector<std::string>;

auto IsNewer(const std::string& f1, const std::string& f2) -> bool;

template<class OnArg>
bool ParseArguments(int argc, char** argv, OnArg onarg)
{
    bool ok = true;
    for (int i = 1; i < argc && ok; ++i) {
        std::string arg(argv[i]);
        auto pos = arg.find('=');
        if (pos != std::string::npos && pos != arg.size() - 1) {
            ok = onarg(arg.substr(0, pos), arg.substr(pos + 1));
        } else {
            ok = onarg(arg);
        }
    }
    return ok;
}
