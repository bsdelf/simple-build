#include "Utils.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <filesystem>
#include <regex>
#include <string>
#include <vector>

// since Apple doesn't complaint with POSIX:2008
#ifdef __APPLE__
#define st_atim st_atimespec
#define st_mtim st_mtimespec
#define st_ctim st_ctimespec
#define st_birthtim st_birthtimespec
#endif

auto DoCmd(const std::string& cmd) -> std::string {
  std::string output;

  FILE* pipe = ::popen(cmd.c_str(), "r");
  if (pipe) {
    while (!::feof(pipe)) {
      char chunk[1024];
      if (::fgets(chunk, sizeof(chunk), pipe) != nullptr) {
        output += chunk;
      }
    }
    ::pclose(pipe);
  }

  return output;
}

auto RegexSplit(const std::string& str, const std::string& pattern) -> std::vector<std::string> {
  std::regex re{pattern};
  std::sregex_token_iterator
    first{str.begin(), str.end(), re, -1},
    last;
  return {first, last};
}

bool IsNewer(const std::string& p1, const std::string& p2) {
  return std::filesystem::last_write_time(p1) > std::filesystem::last_write_time(p2);
}
