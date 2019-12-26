#pragma once

#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

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
std::string JoinStringsImpl(T&& strs, const std::string& separator) {
  auto iter = strs.begin();
  if (iter == strs.end()) {
    return "";
  }
  std::string result = *iter++;
  for (; iter != strs.end(); ++iter) {
    if (!iter->empty()) {
      if (!result.empty()) {
        result += separator;
      }
      result += *iter;
    }
  }
  return result;
}

inline std::string JoinStrings(std::initializer_list<std::string> strs, const std::string& separator = " ") {
  return JoinStringsImpl(std::move(strs), separator);
}

template <class T>
std::string JoinStrings(T&& strs, const std::string& separator = " ") {
  return JoinStringsImpl(std::forward<T>(strs), separator);
}

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