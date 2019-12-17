#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
using namespace std;

#include "scx/Dir.h"
#include "scx/FileInfo.h"
using namespace scx;

#include "Compiler.h"
#include "TransUnit.h"
#include "Utils.h"

int main(int argc, char* argv[]) {
  std::vector<KeyValueArgs::Command> cmds = {
      {"jobs", "set number of jobs", KeyValueArgs::Setter(), "0"},
      {"out", "set output binary", KeyValueArgs::Setter(), "b.out"},
      {"workdir", "set work directory", KeyValueArgs::Setter(), "."},
      {},
      {"as", "set assembler", KeyValueArgs::Setter(), "as"},
      {"asflags", "add assembler flags", KeyValueArgs::Jointer(), ""},
      {"cc", "set c compiler", KeyValueArgs::Setter(), "cc"},
      {"cflags", "add c compiler flags", KeyValueArgs::Jointer(), ""},
      {"cxx", "set c++ compiler", KeyValueArgs::Setter(), "c++"},
      {"cxxflags", "add c++ compiler flags", KeyValueArgs::Jointer(), ""},
      {"flags", "add c & c++ compiler flags", KeyValueArgs::KeyJointer({"cflags", "cxxflags"})},
      {"ld", "set linker", KeyValueArgs::Setter(), "cc"},
      {"ldflags", "add linker flags", KeyValueArgs::Jointer(), ""},
      {"ldorder", "set link order", KeyValueArgs::Setter(), ""},
      {"prefix", "add search directories", KeyValueArgs::KeyValueJointer({{"flags", [](const std::string& str) { return "-I" + str + "/include"; }}, {"ldflags", [](const std::string& str) { return "-L" + str + "/lib"; }}})},
      {},
      {"verbose", "verbose output", KeyValueArgs::ValueSetter("1"), "0"},
      {"clean", "remove output files", KeyValueArgs::ValueSetter("1"), "0"},
      {"nolink", "do not link", KeyValueArgs::ValueSetter("1"), "0"},
      {"thread", "link against pthread", KeyValueArgs::KeyValueJointer({{"flags", "-pthread"},
#ifndef __APPLE__
                                                                        {"ldflags", "-pthread"}
#endif
                                         })},
      {"shared", "build shared library", KeyValueArgs::KeyValueJointer({{"flags", "-fPIC"}, {"ldflags", "-shared"}})},
      {"pipe", "enable -pipe", KeyValueArgs::KeyValueJointer({{"flags", "-pipe"}})},
      {"debug", "enable -g", KeyValueArgs::KeyValueJointer({{"flags", "-g"}})},
      {"release", "enable -DNDEBUG", KeyValueArgs::KeyValueJointer({{"flags", "-DNDEBUG"}})},
      {"strict", "enable -Wall -Wextra -Werror", KeyValueArgs::KeyValueJointer({{"flags", "-Wall -Wextra -Werror"}})},
      {"c89", "enable -std=c89", KeyValueArgs::KeyValueJointer({{"cflags", "-std=c89"}})},
      {"c99", "enable -std=c99", KeyValueArgs::KeyValueJointer({{"cflags", "-std=c99"}})},
      {"c11", "enable -std=c11", KeyValueArgs::KeyValueJointer({{"cflags", "-std=c11"}})},
      {"c++11", "enable -std=c++11", KeyValueArgs::KeyValueJointer({{"cxxflags", "-std=c++11"}})},
      {"c++1y", "enable -std=c++1y", KeyValueArgs::KeyValueJointer({{"cxxflags", "-std=c++1y"}})},
      {"c++14", "enable -std=c++14", KeyValueArgs::KeyValueJointer({{"cxxflags", "-std=c++14"}})},
      {"c++1z", "enable -std=c++1z", KeyValueArgs::KeyValueJointer({{"cxxflags", "-std=c++1z"}})},
      {"c++17", "enable -std=c++17", KeyValueArgs::KeyValueJointer({{"cxxflags", "-std=c++17"}})},
  };

  // parse command line arguments
  vector<string> inputPaths;
  auto args = KeyValueArgs::Parse(argc, argv, cmds, [&](std::string&& key, std::string&& value) {
    if (key == "help") {
      const size_t n = 8;
      const std::string blank(n, ' ');
      cout << "Usage:" << endl;
      cout << blank << std::string(argv[0]) << " [option ...] [file ...] [directory ...]" << endl;
      cout << endl;
      cout << KeyValueArgs::ToString(cmds, n) << endl;
      ::exit(EXIT_SUCCESS);
    } else if (value.empty()) {
      inputPaths.push_back(key);
    } else {
      cerr << "Invalid argument: " << key;
      if (!value.empty()) {
        cerr << "=" << value;
      }
      cerr << endl;
      ::exit(EXIT_FAILURE);
    }
  });
  if (inputPaths.empty()) {
    inputPaths.push_back(".");
  }

  // scan source files
  vector<string> sourcePaths;
  for (const auto& path : inputPaths) {
    switch (FileInfo(path).Type()) {
      case FileType::Regular: {
        sourcePaths.push_back(path);
        break;
      };
      case FileType::Directory: {
        auto subpaths = Dir::ListDir(path);
        sourcePaths.reserve(sourcePaths.size() + subpaths.size());
        for (const auto& subpath : subpaths) {
          const auto baseName = FileInfo::BaseName(subpath);
          if (baseName == "." || baseName == "..") {
            continue;
          }
          const auto filePath = path + "/" + subpath;
          sourcePaths.push_back(filePath);
        }
        break;
      };
      default: {
        cerr << "Invalid argument: " << path << endl;
        ::exit(EXIT_FAILURE);
      }
    }
  }
  std::sort(sourcePaths.begin(), sourcePaths.end(), [](const auto& a, const auto& b) {
    return std::strcoll(a.c_str(), b.c_str()) < 0 ? true : false;
  });
  if (sourcePaths.empty()) {
    cerr << "FATAL: nothing to build!" << endl;
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
        cerr << "Failed to create directory!" << endl;
        cerr << "    Directory: " << dir << endl;
        ::exit(EXIT_FAILURE);
      }
    } else if (info.Type() != FileType::Directory) {
      cerr << "Bad work directory! " << endl;
      cerr << "    Directory: " << dir << endl;
      cerr << "    File type: " << FileType::ToString(info.Type()) << endl;
      ::exit(EXIT_FAILURE);
    }
  }

  // scan translation units
  vector<TransUnit> newUnits;
  string allObjects;
  {
    bool hasCpp = false;
    const auto& ldorder = args["ldorder"];
    std::vector<std::pair<size_t, std::string>> headObjects;
    for (const auto& file : sourcePaths) {
      const auto& outdir = args["workdir"];
      bool isCpp = false;
      auto unit = TransUnit::Make(file, outdir, args, isCpp);
      if (!unit) {
        continue;
      }
      hasCpp = hasCpp || isCpp;
      auto pos = ldorder.empty() ? std::string::npos : ldorder.find(FileInfo::BaseName(unit.objfile));
      if (pos != std::string::npos) {
        headObjects.emplace_back(pos, unit.objfile);
      } else {
        allObjects += unit.objfile + ' ';
      }
      if (!unit.command.empty()) {
        newUnits.push_back(std::move(unit));
      }
    }
    std::sort(headObjects.begin(), headObjects.end(), [](const auto& a, const auto& b) {
      return a.first > b.first;
    });
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
    cout << "New translation units: " << endl;
    cout << split << endl;
    for (const auto& unit : newUnits) {
      cout << unit.Note(true) << endl;
      for (const auto& dep : unit.deps) {
        cout << dep << ", ";
      }
      cout << endl
           << split << endl;
    }
  }
#endif

  // clean
  if (args["clean"] == "1") {
    const string& cmd = "rm -f " + args["workdir"] + args["out"] + ' ' + allObjects;
    cout << cmd << endl;
    ::system(cmd.c_str());
    ::exit(EXIT_SUCCESS);
  }

  // compile
  auto verbose = args["verbose"] == "1";
  if (!newUnits.empty()) {
    cout << "* Build: ";
    if (verbose) {
      for (size_t i = 0; i < newUnits.size(); ++i) {
        cout << newUnits[i].srcfile << ((i + 1 < newUnits.size()) ? ", " : "");
      }
    } else {
      cout << newUnits.size() << (newUnits.size() > 1 ? " files" : " file");
    }
    cout << endl;
    if (Compiler::Run(newUnits, std::stoi(args["jobs"]), verbose) != 0) {
      ::exit(EXIT_FAILURE);
    }
  }

  // link
  bool hasOut = FileInfo(args["workdir"] + args["out"]).Exists();
  if ((!hasOut || !newUnits.empty()) && args["nolink"] != "1") {
    string ldCmd = Join({args["ld"], args["ldflags"], "-o", args["workdir"] + args["out"], allObjects});
    if (verbose) {
      cout << "- Link - " << ldCmd << endl;
    } else {
      cout << "- Link - " << args["workdir"] + args["out"] << endl;
    }
    if (::system(ldCmd.c_str()) != 0) {
      cerr << "FATAL: failed to link!" << endl;
      cerr << "    file:   " << allObjects << endl;
      cerr << "    ld:     " << args["ld"] << endl;
      cerr << "    ldflags: " << args["ldflags"] << endl;
      ::exit(EXIT_FAILURE);
    }
  }

  ::exit(EXIT_SUCCESS);
}
