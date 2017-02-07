#pragma once

#include <string>
#include <vector>

auto DoCmd(const std::string& cmd) -> std::string;

auto RegexSplit(const std::string& str, const std::string& pattern) -> std::vector<std::string>;

auto IsNewer(const std::string& f1, const std::string& f2) -> bool;

template<class OnFlag, class OnKeyValue>
void ParseArguments(int argc, char** argv, OnFlag onFlag, OnKeyValue onKeyValue)
{
    bool cont = true;
    for (int i = 1; i < argc && cont; ++i) {
        std::string arg(argv[i]);
        auto pos = arg.find('=');
        if (pos != std::string::npos && pos != arg.size() - 1) {
            std::string key(arg.substr(0, pos));
            std::string val(arg.substr(pos + 1));
            cont = onKeyValue(std::move(key), std::move(val));
        } else {
            cont = onFlag(std::move(arg));
        }
    }
}
