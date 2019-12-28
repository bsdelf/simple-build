#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

struct SourceFile {
  std::string source;
  std::string output;
  std::vector<std::string> dependencies;
  std::string command;

  explicit operator bool() const {
    return !source.empty() || !output.empty();
  }
};

class SourceAnalyzer {
  using Handler = SourceFile (SourceAnalyzer::*)(const std::string&) const;

 public:
  explicit SourceAnalyzer(std::map<std::string, std::string>& args)
    : args_(args) {
    auto install = [this](Handler handler, const char* extensions[]) {
      for (size_t i = 0; extensions[i]; ++i) {
        handlers_.emplace(extensions[i], handler);
      }
    };
    install(&SourceAnalyzer::ProcessC, (const char*[]){".c", nullptr});
    install(&SourceAnalyzer::ProcessCpp, (const char*[]){".cc", ".cpp", ".cxx", "c++", nullptr});
    install(&SourceAnalyzer::ProcessAsm, (const char*[]){".s", ".asm", ".nas", nullptr});
  }

  SourceFile Process(const std::string& path) const;

 private:
  SourceFile ProcessC(const std::string& path) const;
  SourceFile ProcessCpp(const std::string& path) const;
  SourceFile ProcessAsm(const std::string& path) const;

 private:
  std::map<std::string_view, Handler> handlers_;
  std::map<std::string, std::string>& args_;
};
