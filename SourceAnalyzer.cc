#include "SourceAnalyzer.h"
#include "Utils.h"

#include <algorithm>
#include <filesystem>
#include <vector>

static auto ShouldCompile(
  const std::string& output,
  const std::vector<std::string>& dependencies) -> bool {
  if (!std::filesystem::exists(output)) {
    return true;
  }
  return std::any_of(
    dependencies.begin(),
    dependencies.end(),
    [&](const auto& dependency) { return IsNewer(dependency, output); });
}

static auto GetDepfiles(
  const std::string& compiler,
  const std::string& flags,
  const std::string& source) -> std::vector<std::string> {
  const auto& command = JoinStrings({compiler, "-MM", source, flags});
  const auto& output = RunCommand(command);
  auto dependencies = RegexSplit(output, R"((\s)+(\\)*(\s)*)");
  if (dependencies.size() <= 1) {
    return {};
  }
  dependencies.erase(dependencies.begin());
  return dependencies;
}

static auto BuildOutputPath(const std::string& workdir, const std::string& source) -> std::string {
  auto filename = std::filesystem::path(source).filename();
  return (std::filesystem::path(workdir) / filename).string() + ".o";
}

auto SourceAnalyzer::Process(const std::string& source) const -> SourceFile {
  const auto& path = std::filesystem::path(source);
  if (!path.has_extension()) {
    return {};
  }
  const auto& extension = ToLower(path.extension().string());
  const auto& iter = handlers_.find(extension);
  if (iter == handlers_.end()) {
    return {};
  }
  return (this->*(iter->second))(source);
}

auto SourceAnalyzer::ProcessC(const std::string& source) const -> SourceFile {
  const auto& compiler = args_.at("cc");
  const auto& flags = args_.at("cflags");
  const auto& depfiles = GetDepfiles(compiler, flags, source);
  auto output = BuildOutputPath(args_.at("workdir"), source);
  std::string command;
  if (ShouldCompile(output, depfiles)) {
    command = JoinStrings({compiler, flags, "-o", output, "-c", source});
  }
  return {source, std::move(output), depfiles, std::move(command), Linker::ForC(compiler)};
}

auto SourceAnalyzer::ProcessCpp(const std::string& source) const -> SourceFile {
  const auto& compiler = args_.at("cxx");
  const auto& flags = args_.at("cxxflags");
  const auto& depfiles = GetDepfiles(compiler, flags, source);
  auto output = BuildOutputPath(args_.at("workdir"), source);
  std::string command;
  if (ShouldCompile(output, depfiles)) {
    command = JoinStrings({compiler, flags, "-o", output, "-c", source});
  }
  return {source, std::move(output), depfiles, std::move(command), Linker::ForCpp(compiler)};
}

auto SourceAnalyzer::ProcessAsm(const std::string& source) const -> SourceFile {
  const auto& compiler = args_.at("as");
  const auto& flags = args_.at("asflags");
  auto output = BuildOutputPath(args_.at("workdir"), source);
  std::string command;
  if (ShouldCompile(output, {source})) {
    command = JoinStrings({compiler, flags, "-o", output, source});
  }
  return {source, std::move(output), {source}, std::move(command), Linker::ForAsm(compiler)};
}