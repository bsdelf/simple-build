#include "SourceAnalyzer.h"
#include "Utils.h"

#include <algorithm>
#include <filesystem>
#include <vector>

static bool ShouldCompile(
  const std::string& output,
  const std::vector<std::string>& dependencies) {
  if (!std::filesystem::exists(output)) {
    return true;
  }
  return std::any_of(
    dependencies.begin(),
    dependencies.end(),
    [&](const auto& dependency) { return IsNewer(dependency, output); });
}

static std::vector<std::string> GetDepfiles(
  const std::string& compiler,
  const std::string& flags,
  const std::string& source) {
  const auto& output = RunCommand(compiler + " -MM " + source + " " + flags);
  auto dependencies = RegexSplit(output, "(\\s)+(\\\\)*(\\s)*");
  if (dependencies.size() <= 1) {
    return {};
  }
  dependencies.erase(dependencies.begin());
  return dependencies;
}

static std::string BuildOutputPath(const std::string& workdir, const std::string& source) {
  auto filename = std::filesystem::path(source).filename();
  return (std::filesystem::path(workdir) / filename).string() + ".o";
}

SourceFile SourceAnalyzer::Process(const std::string& path) const {
  const auto& p = std::filesystem::path(path);
  if (!p.has_extension()) {
    return {};
  }
  const auto& extension = ToLower(p.extension().string());
  const auto& iter = handlers_.find(extension);
  if (iter == handlers_.end()) {
    return {};
  }
  return (this->*(iter->second))(path);
}

SourceFile SourceAnalyzer::ProcessC(const std::string& source) const {
  const auto& compiler = args_.at("cc");
  const auto& flags = args_.at("cflags");
  const auto& depfiles = GetDepfiles(compiler, flags, source);
  auto output = BuildOutputPath(args_.at("workdir"), source);
  std::string command;
  if (ShouldCompile(output, depfiles)) {
    command = compiler + " " + flags + " -o " + output + " -c " + source;
  }
  return {source, std::move(output), depfiles, std::move(command)};
}

SourceFile SourceAnalyzer::ProcessCpp(const std::string& source) const {
  const auto& compiler = args_.at("cxx");
  const auto& flags = args_.at("cxxflags");
  const auto& depfiles = GetDepfiles(compiler, flags, source);
  auto output = BuildOutputPath(args_.at("workdir"), source);
  std::string command;
  if (ShouldCompile(output, depfiles)) {
    command = compiler + " " + flags + " -o " + output + " -c " + source;
  }
  args_.at("ld") = compiler;
  return {source, std::move(output), depfiles, std::move(command)};
}

SourceFile SourceAnalyzer::ProcessAsm(const std::string& source) const {
  auto output = BuildOutputPath(args_.at("workdir"), source);
  std::string command;
  if (ShouldCompile(output, {source})) {
    const auto& compiler = args_.at("as");
    const auto& flags = args_.at("asflags");
    command = compiler + " " + flags + " -o " + output + " " + source;
  }
  return {source, std::move(output), {source}, std::move(command)};
}