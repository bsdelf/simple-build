#pragma once

#include <filesystem>
#include <string>
#include <vector>

auto DoCmd(const std::string& cmd) -> std::string;

auto RegexSplit(const std::string& str, const std::string& pattern) -> std::vector<std::string>;

bool IsNewer(const std::string& p1, const std::string& p2);

inline auto Join(std::initializer_list<std::string> lst, const std::string& data = " ") -> std::string {
  auto iter = lst.begin();
  if (iter == lst.end()) {
    return "";
  }
  std::string result = *iter++;
  for (; iter != lst.end(); ++iter) {
    if (!iter->empty()) {
      result += data + *iter;
    }
  }
  return result;
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