#pragma once

#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>
#include <cstring>

inline auto ToLower(std::string str) {
  std::transform(
    str.begin(), str.end(), str.begin(),
    [](auto c) { return std::tolower(c); });
  return str;
}

template <class T>
void SortStrings(T&& strs) {
  std::sort(strs.begin(), strs.end(), [](const auto& a, const auto& b) {
    return std::strcoll(a.c_str(), b.c_str()) < 0;
  });
}

template <class T>
auto JoinStringsImpl(T&& strs, const std::string& separator) -> std::string {
  std::string result;
  for (const auto& str : strs) {
    if (!str.empty()) {
      if (!result.empty()) {
        result += separator;
      }
      result += str;
    }
  }
  return result;
}

inline auto JoinStrings(
  std::initializer_list<std::string_view> strs,
  const std::string& separator = " ") -> std::string {
  return JoinStringsImpl(strs, separator);
}

template <class T>
auto JoinStrings(T&& strs, const std::string& separator = " ") -> std::string {
  return JoinStringsImpl(std::forward<T>(strs), separator);
}

auto RegexSplit(const std::string& str, const std::string& pattern) -> std::vector<std::string>;

auto RunCommand(const std::string& cmd) -> std::string;

inline auto IsNewer(const std::string& p1, const std::string& p2) -> bool {
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
