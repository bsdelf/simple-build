#include "TransUnit.h"
#include "Utils.h"

#include <algorithm>
#include <filesystem>
#include <unordered_set>

static const std::unordered_set<std::string> C_EXT = {".c"};
static const std::unordered_set<std::string> CXX_EXT = {".cc", ".cxx", ".cpp", ".C"};
static const std::unordered_set<std::string> ASM_EXT = {".s", ".S", ".asm", ".nas"};

// Maybe we can cache mtime to reduce the stat() system calls.
TransUnit MakeForC(const std::string& srcfile, const std::string& objfile, const std::string& compiler, const std::string& flags) {
  auto depfiles = RegexSplit(
    DoCmd(compiler + " -MM " + srcfile + " " + flags),
    "(\\s)+(\\\\)*(\\s)*");
  if (depfiles.size() <= 1) {
    return {};
  }
  depfiles.erase(depfiles.begin());

  const auto required =
    std::filesystem::exists(objfile)
      ? std::any_of(
          depfiles.begin(),
          depfiles.end(),
          [&](const auto& dep) { return IsNewer(dep, objfile); })
      : true;

  std::string command;
  if (required) {
    command = compiler + " " + flags + " -o " + objfile + " -c " + srcfile;
  }
#if !defined(DEBUG)
  return {srcfile, objfile, command};
#else
  return {srcfile, objfile, command, depfiles};
#endif
}

TransUnit MakeForAsm(const std::string& srcfile, const std::string& objfile, const std::string& compiler, const std::string& flags) {
  std::string command;
  if (!std::filesystem::exists(objfile) || IsNewer(srcfile, objfile)) {
    command = compiler + " " + flags + " -o " + objfile + " " + srcfile;
  }
#if !defined(DEBUG)
  return {srcfile, objfile, command};
#else
  return {srcfile, objfile, command, {}};
#endif
}

auto TransUnit::Make(const std::string& file, const std::string& outdir, const std::map<std::string, std::string>& args, bool& is_cpp) -> TransUnit {
  if (!std::filesystem::exists(file) || !std::filesystem::is_regular_file(file)) {
    return {};
  }
  const auto path = std::filesystem::path(file);
  std::string objfile = (std::filesystem::path(outdir) / path.filename()).string() + ".o";
  std::string ext = path.extension();
  std::string compiler;
  std::string flags;
  if (C_EXT.find(ext) != C_EXT.end()) {
    compiler = args.at("cc");
    flags = args.at("cflags");
    is_cpp = false;
    return MakeForC(file, objfile, compiler, flags);
  } else if (CXX_EXT.find(ext) != CXX_EXT.end()) {
    compiler = args.at("cxx");
    flags = args.at("cxxflags");
    is_cpp = true;
    return MakeForC(file, objfile, compiler, flags);
  } else if (ASM_EXT.find(ext) != ASM_EXT.end()) {
    compiler = args.at("as");
    flags = args.at("asflags");
    is_cpp = false;
    return MakeForAsm(file, objfile, compiler, flags);
  } else {
    return {};
  }
}

TransUnit::operator bool() const {
  return !srcfile.empty() || !objfile.empty();
}

auto TransUnit::Note(bool verbose) const -> std::string {
  return verbose ? command : (srcfile + " => " + objfile);
}
