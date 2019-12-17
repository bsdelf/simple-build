#pragma once

#include <string>
#include <vector>

auto DoCmd(const std::string& cmd) -> std::string;

auto RegexSplit(const std::string& str, const std::string& pattern) -> std::vector<std::string>;

auto IsNewer(const std::string& f1, const std::string& f2) -> bool;

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
