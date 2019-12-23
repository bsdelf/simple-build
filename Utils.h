#pragma once

#include <filesystem>
#include <initializer_list>
#include <string>
#include <vector>

inline auto ToLower(std::string str) {
  std::transform(
    str.begin(), str.end(), str.begin(),
    [](auto c) { return std::tolower(c); });
  return str;
}

std::string JoinStrings(std::initializer_list<std::string> strs, const std::string& data = " ");

std::vector<std::string> RegexSplit(const std::string& str, const std::string& pattern);

std::string RunCommand(const std::string& cmd);

inline bool IsNewer(const std::string& p1, const std::string& p2) {
  return std::filesystem::last_write_time(p1) > std::filesystem::last_write_time(p2);
}

template <class Filter, class Collector>
void WalkDirectory(const std::string& path, Filter filter, Collector collector) {
  for (std::vector<std::string> stack{path}; !stack.empty();) {
    auto dir = stack.back();
    stack.pop_back();
    for (const auto& iter : std::filesystem::directory_iterator(dir)) {
      const auto& path = iter.path();
      if (filter(path)) {
        if (std::filesystem::is_directory(path)) {
          stack.push_back(path);
        }
        collector(path);
      }
    }
  }
}