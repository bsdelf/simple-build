#pragma once

#include <string>
#include <vector>

auto DoCmd(const std::string& cmd) -> std::string;

auto RegexSplit(const std::string& in, const std::string& sre) -> std::vector<std::string>;

auto IsNewer(const std::string& f1, const std::string& f2) -> bool;
