#include "Utils.h"

#include <stdio.h>

#include <filesystem>
#include <regex>
#include <string>
#include <vector>

std::string JoinStrings(std::initializer_list<std::string> strs, const std::string& data) {
  auto iter = strs.begin();
  if (iter == strs.end()) {
    return "";
  }
  std::string result = *iter++;
  for (; iter != strs.end(); ++iter) {
    if (!iter->empty()) {
      result += data + *iter;
    }
  }
  return result;
}

std::vector<std::string> RegexSplit(const std::string& str, const std::string& pattern) {
  std::regex re{pattern};
  std::sregex_token_iterator
    first{str.begin(), str.end(), re, -1},
    last;
  return {first, last};
}

std::string RunCommand(const std::string& cmd) {
  std::string output;
  FILE* pipe = ::popen(cmd.c_str(), "r");
  if (pipe) {
    while (!::feof(pipe)) {
      char buffer[1024] = {0};
      if (::fgets(buffer, sizeof(buffer), pipe)) {
        output += buffer;
      }
    }
    ::pclose(pipe);
  }
  return output;
}
