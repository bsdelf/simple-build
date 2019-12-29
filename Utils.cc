#include <array>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "Utils.h"

auto RegexSplit(const std::string& str, const std::string& pattern) -> std::vector<std::string> {
  std::regex re{pattern};
  std::sregex_token_iterator
    first{str.begin(), str.end(), re, -1},
    last;
  return {first, last};
}

auto RunCommand(const std::string& cmd) -> std::string {
  std::string result;
  std::unique_ptr<FILE, decltype(&::pclose)> pipe(::popen(cmd.c_str(), "r"), ::pclose);
  if (!pipe) {
    throw std::runtime_error("popen() failed");
  }
  std::array<char, 128> buffer{};
  while (::fgets(buffer.data(), buffer.size(), pipe.get())) {
    result += buffer.data();
  }
  return result;
}
