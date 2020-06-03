#pragma once

#include <array>
#include <map>
#include <string>
#include <string_view>
#include <vector>

struct Linker {
  int priority = -1;
  std::string command;

  bool operator>(const Linker& that) const {
    return priority > that.priority;
  }

  explicit operator bool() const {
    return priority >= 0 && !command.empty();
  }

  static auto FromAsm(const std::string& command) -> Linker {
    const auto priority = command.empty() ? -1 : 1;
    return {priority, command};
  }

  static auto FromC(const std::string& command) -> Linker {
    const auto priority = command.empty() ? -1 : 2;
    return {priority, command};
  }

  static auto FromCpp(const std::string& command) -> Linker {
    const auto priority = command.empty() ? -1 : 3;
    return {priority, command};
  }

  static auto FromLd(const std::string& command) -> Linker {
    const auto priority = command.empty() ? -1 : 4;
    return {priority, command};
  }
};

struct SourceFile {
  std::string source;
  std::string output;
  std::vector<std::string> dependencies;
  std::string command;
  Linker linker;

  explicit operator bool() const {
    return !source.empty() || !output.empty();
  }
};

class SourceAnalyzer {
  using Handler = SourceFile (SourceAnalyzer::*)(const std::string&) const;

 public:
  explicit SourceAnalyzer(const std::map<std::string, std::string>& args)
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
  const std::map<std::string, std::string>& args_;
  std::map<std::string_view, Handler> handlers_;
};
