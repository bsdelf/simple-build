#pragma once

#include <string>
#include <vector>

auto DoCmd(const std::string& cmd) -> std::string;

auto RegexSplit(const std::string& str, const std::string& pattern) -> std::vector<std::string>;

auto IsNewer(const std::string& f1, const std::string& f2) -> bool;
