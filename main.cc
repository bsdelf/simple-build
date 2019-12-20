#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "scx/Dir.h"
#include "scx/FileInfo.h"
using namespace scx;

#include "Compiler.h"
#include "TransUnit.h"
#include "Utils.h"

int main(int argc, char* argv[]) {
  std::vector<std::string> input_paths;

  ArgumentParser parser;
  parser
    .On("clean", "clean files", ArgumentParser::Set("0", "1"))
    .On("jobs", "set number of jobs", ArgumentParser::Set("0", "0"))
    .On("out", "set output binary", ArgumentParser::Set("a.out", "a.out"))
    .On("workdir", "set work directory", ArgumentParser::Set(".", "."))
    .On("verbose", "enable verbose output", ArgumentParser::Set("0", "1"))
    .Split()
    .On("as", "set assembler", ArgumentParser::Set("as", "as"))
    .On("asflags", "add assembler flags", ArgumentParser::Join({}, {}))
    .On("cc", "set c compiler", ArgumentParser::Set("cc", "cc"))
    .On("cflags", "add c compiler flags", ArgumentParser::Join({}, {}))
    .On("cxx", "set c++ compiler", ArgumentParser::Set("c++", "c++"))
    .On("cxxflags", "add c++ compiler flags", ArgumentParser::Join({}, {}))
    .On("ld", "set linker", ArgumentParser::Set("cc", "cc"))
    .On("ldflags", "add linker flags", ArgumentParser::Join({}, {}))
    .On("ldorder", "set linkage order", ArgumentParser::Set({}, {}))
    .On("prefix", "add search directories",
        ArgumentParser::JoinTo(
          "cflags", {}, {},
          [](auto& input) { input = "-I" + input + "/include"; }),
        ArgumentParser::JoinTo(
          "cxxflags", {}, {},
          [](auto& input) { input = "-I" + input + "/include"; }),
        ArgumentParser::JoinTo(
          "ldflags", {}, {},
          [](auto& input) { input = "-L" + input + "/lib"; }))
    .Split()
    .On("wol", "without link", ArgumentParser::Set("0", "1"))
    .On("thread", "use pthreads",
        ArgumentParser::JoinTo("cflags", {}, "-pthread"),
        ArgumentParser::JoinTo("cxxflags", {}, "-pthread")
#ifndef __APPLE__
          ,
        ArgumentParser::JoinTo("ldflags", {}, "-pthread")
#endif
          )
    .On("optimize", "set optimize level",
        ArgumentParser::JoinTo("cflags", {}, "2", [](auto& input) { input = "-O" + input; }),
        ArgumentParser::JoinTo("cxxflags", {}, "2", [](auto& input) { input = "-O" + input; }))
    .On("debug", "enable -g",
        ArgumentParser::JoinTo("cflags", {}, "-g"),
        ArgumentParser::JoinTo("cxxflags", {}, "-g"))
    .On("release", "enable -DNDEBUG",
        ArgumentParser::JoinTo("cflags", {}, "-DNDEBUG"),
        ArgumentParser::JoinTo("cxxflags", {}, "-DNDEBUG"))
    .On("strict", "enable -Wall -Wextra -Werror",
        ArgumentParser::JoinTo("cflags", {}, "-Wall -Wextra -Werror"),
        ArgumentParser::JoinTo("cxxflags", {}, "-Wall -Wextra -Werror"))
    .On("shared", "enable -fPIC -shared",
        ArgumentParser::JoinTo("cflags", {}, "-fPIC"),
        ArgumentParser::JoinTo("cxxflags", {}, "-fPIC"),
        ArgumentParser::JoinTo("ldflags", {}, "-shared"))
    .On("lto", "enable -flto", ArgumentParser::JoinTo("ldflags", {}, "-flto"))
    .On("c89", "enable -std=c89", ArgumentParser::JoinTo("cflags", {}, "-std=c89"))
    .On("c99", "enable -std=c99", ArgumentParser::JoinTo("cflags", {}, "-std=c99"))
    .On("c11", "enable -std=c11", ArgumentParser::JoinTo("cflags", {}, "-std=c11"))
    .On("c18", "enable -std=c18", ArgumentParser::JoinTo("cflags", {}, "-std=c18"))
    .On("c++11", "enable -std=c++11", ArgumentParser::JoinTo("cxxflags", {}, "-std=c++11"))
    .On("c++14", "enable -std=c++14", ArgumentParser::JoinTo("cxxflags", {}, "-std=c++14"))
    .On("c++17", "enable -std=c++17", ArgumentParser::JoinTo("cxxflags", {}, "-std=c++17"))
    .On("c++20", "enable -std=c++20", ArgumentParser::JoinTo("cxxflags", {}, "-std=c++20"))
    .Split()
    .On("help", "show help", ArgumentParser::Set("0", "1"))
    .OnUnknown([&](std::string key, std::optional<std::string> value) {
      if (value) {
        std::cerr << "Invalid argument: " << key;
        if (value) {
          std::cerr << "=" << *value;
        }
        std::cerr << std::endl;
        ::exit(EXIT_FAILURE);
      } else {
        input_paths.push_back(std::move(key));
      }
    })
    .Parse(argc, argv);

  auto args = parser.Data();
  if (args["help"] == "1") {
    const int n = 8;
    std::cout
      << "Usage:" << std::endl
      << std::string(n, ' ') << std::string(argv[0]) << " [option ...] [file ...] [directory ...]" << std::endl
      << std::endl
      << parser.FormatHelp(ArgumentParser::FormatHelpOptions{.space_before_key = n})
      << std::endl;
    ::exit(EXIT_SUCCESS);
  }

  if (input_paths.empty()) {
    input_paths.push_back(".");
  }

  // scan source files
  std::vector<std::string> source_paths;
  for (const auto& path : input_paths) {
    switch (FileInfo(path).Type()) {
      case FileType::Regular: {
        source_paths.push_back(path);
        break;
      };
      case FileType::Directory: {
        auto subpaths = Dir::ListDir(path);
        source_paths.reserve(source_paths.size() + subpaths.size());
        for (const auto& subpath : subpaths) {
          const auto baseName = FileInfo::BaseName(subpath);
          if (baseName == "." || baseName == "..") {
            continue;
          }
          const auto filePath = path + "/" + subpath;
          source_paths.push_back(filePath);
        }
        break;
      };
      default: {
        std::cerr << "Invalid path: " << path << std::endl;
        ::exit(EXIT_FAILURE);
      }
    }
  }
  std::sort(source_paths.begin(), source_paths.end(), [](const auto& a, const auto& b) {
    return std::strcoll(a.c_str(), b.c_str()) < 0 ? true : false;
  });
  if (source_paths.empty()) {
    std::cerr << "FATAL: nothing to build!" << std::endl;
    ::exit(EXIT_FAILURE);
  }

  // ensure work directory
  if (!args["workdir"].empty()) {
    auto& dir = args["workdir"];
    if (dir[dir.size() - 1] != '/') {
      dir += "/";
    }
    const auto& info = FileInfo(dir);
    if (!info.Exists()) {
      if (!Dir::MakeDir(dir, 0744)) {
        std::cerr << "Failed to create directory!" << std::endl;
        std::cerr << "    Directory: " << dir << std::endl;
        ::exit(EXIT_FAILURE);
      }
    } else if (info.Type() != FileType::Directory) {
      std::cerr << "Bad work directory! " << std::endl;
      std::cerr << "    Directory: " << dir << std::endl;
      std::cerr << "    File type: " << FileType::ToString(info.Type()) << std::endl;
      ::exit(EXIT_FAILURE);
    }
  }

  // scan translation units
  std::vector<TransUnit> newUnits;
  std::string allObjects;
  {
    bool hasCpp = false;
    const auto& ldorder = args["ldorder"];
    std::vector<std::pair<size_t, std::string>> headObjects;
    for (const auto& file : source_paths) {
      const auto& outdir = args["workdir"];
      bool isCpp = false;
      auto unit = TransUnit::Make(file, outdir, args, isCpp);
      if (!unit) {
        continue;
      }
      hasCpp = hasCpp || isCpp;
      auto pos =
        ldorder.empty() ? std::string::npos : ldorder.find(FileInfo::BaseName(unit.objfile));
      if (pos != std::string::npos) {
        headObjects.emplace_back(pos, unit.objfile);
      } else {
        allObjects += unit.objfile + ' ';
      }
      if (!unit.command.empty()) {
        newUnits.push_back(std::move(unit));
      }
    }
    std::sort(headObjects.begin(), headObjects.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    for (auto&& headObject : headObjects) {
      allObjects.insert(0, std::move(headObject.second) + ' ');
    }
    if (hasCpp) {
      args["ld"] = args["cxx"];
    }
  }

#ifdef DEBUG
  // Debug info.
  {
    std::string split(80, '-');
    cout << "New translation units: " << std::endl;
    cout << split << std::endl;
    for (const auto& unit : newUnits) {
      cout << unit.Note(true) << std::endl;
      for (const auto& dep : unit.deps) {
        cout << dep << ", ";
      }
      cout << std::endl
           << split << std::endl;
    }
  }
#endif

  // clean
  if (args["clean"] == "1") {
    const std::string& cmd = "rm -f " + args["workdir"] + args["out"] + ' ' + allObjects;
    std::cout << cmd << std::endl;
    ::system(cmd.c_str());
    ::exit(EXIT_SUCCESS);
  }

  // compile
  auto verbose = args["verbose"] == "1";
  if (!newUnits.empty()) {
    std::cout << "* Build: ";
    if (verbose) {
      for (size_t i = 0; i < newUnits.size(); ++i) {
        std::cout << newUnits[i].srcfile << ((i + 1 < newUnits.size()) ? ", " : "");
      }
    } else {
      std::cout << newUnits.size() << (newUnits.size() > 1 ? " files" : " file");
    }
    std::cout << std::endl;
    if (Compiler::Run(newUnits, std::stoi(args["jobs"]), verbose) != 0) {
      ::exit(EXIT_FAILURE);
    }
  }

  // link
  bool hasOut = FileInfo(args["workdir"] + args["out"]).Exists();
  if ((!hasOut || !newUnits.empty()) && args["wol"] != "1") {
    std::string ldCmd =
      Join({args["ld"], args["ldflags"], "-o", args["workdir"] + args["out"], allObjects});
    if (verbose) {
      std::cout << "- Link - " << ldCmd << std::endl;
    } else {
      std::cout << "- Link - " << args["workdir"] + args["out"] << std::endl;
    }
    if (::system(ldCmd.c_str()) != 0) {
      std::cerr << "FATAL: failed to link!" << std::endl;
      std::cerr << "    file:   " << allObjects << std::endl;
      std::cerr << "    ld:     " << args["ld"] << std::endl;
      std::cerr << "    ldflags: " << args["ldflags"] << std::endl;
      ::exit(EXIT_FAILURE);
    }
  }

  ::exit(EXIT_SUCCESS);
}
