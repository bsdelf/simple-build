#pragma once

#include <array>
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
  explicit SourceAnalyzer(std::map<std::string, std::string>* args)
    : args_(args) {
    auto install = [this](Handler handler, auto extensions) {
      for (auto extension : extensions) {
        handlers_.emplace(extension, handler);
      }
    };
    install(&SourceAnalyzer::ProcessC, std::array{".c"});
    install(&SourceAnalyzer::ProcessCpp, std::array{".cc", ".cpp", ".cxx", ".c++"});
    install(&SourceAnalyzer::ProcessAsm, std::array{".s", ".asm", ".nas"});
  }

  [[nodiscard]] auto Process(const std::string& path) const -> SourceFile;

 private:
  [[nodiscard]] auto ProcessC(const std::string& path) const -> SourceFile;
  [[nodiscard]] auto ProcessCpp(const std::string& path) const -> SourceFile;
  [[nodiscard]] auto ProcessAsm(const std::string& path) const -> SourceFile;

 private:
  std::map<std::string_view, Handler> handlers_;
  std::map<std::string, std::string>* args_;
};
