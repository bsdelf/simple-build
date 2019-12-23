#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "ArgumentParser.h"
#include "Executor.h"
#include "SourceAnalyzer.h"
#include "Utils.h"

#include "BlockingQueue.h"
#include "Semaphore.h"
using namespace ccbb;

int main(int argc, char* argv[]) {
  Executor executor;
  executor.Start();

  std::vector<std::string> input_paths;

  ArgumentParser parser;
  parser
    .On("clean", "clean files", ArgumentParser::Set("0", "1"))
    .On("jobs", "set number of jobs", ArgumentParser::Set("0", "0"))
    .On("target", "set target name", ArgumentParser::Set("a.out", "a.out"))
    .On("workdir", "set working directory", ArgumentParser::Set(".", "."))
    .On("verbose", "enable verbose output", ArgumentParser::Set("0", "1"))
    .Split()
    .On("as", "set assembler", ArgumentParser::Set("as", "as"))
    .On("asflags", "add assembler flags", ArgumentParser::Join("", {}))
    .On("cc", "set c compiler", ArgumentParser::Set("cc", "cc"))
    .On("cflags", "add c compiler flags", ArgumentParser::Join("", {}))
    .On("cxx", "set c++ compiler", ArgumentParser::Set("c++", "c++"))
    .On("cxxflags", "add c++ compiler flags", ArgumentParser::Join("", {}))
    .On("ld", "set linker", ArgumentParser::Set("cc", "cc"))
    .On("ldflags", "add linker flags", ArgumentParser::Join("", {}))
    .On("ldorder", "set linkage order", ArgumentParser::Set("", {}))
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
      if (!value &&
          (std::filesystem::is_regular_file(key) ||
           std::filesystem::is_directory(key))) {
        input_paths.push_back(std::move(key));
      } else {
        std::cerr << "(E) invalid argument: " << key;
        if (value) {
          std::cerr << "=" << *value;
        }
        std::cerr << std::endl;
        ::exit(EXIT_FAILURE);
      }
    })
    .Parse(argc, argv);

  auto args = parser.Data();
  const auto is_verbose = args.at("verbose") == "1";
  const auto target = std::filesystem::path(args.at("workdir")) / std::filesystem::path(args.at("target"));

  // show help
  if (args.at("help") == "1") {
    const int n = 8;
    std::cout
      << "Usage:" << std::endl
      << std::string(n, ' ') << std::string(argv[0]) << " [option ...] [file ...] [directory ...]" << std::endl
      << std::endl
      << parser.FormatHelp(ArgumentParser::FormatHelpOptions{.space_before_key = n})
      << std::endl;
    ::exit(EXIT_SUCCESS);
  }

  // ensure working directory
  {
    const std::filesystem::path& dir = args.at("workdir");
    const auto& status = std::filesystem::status(dir);
    if (status.type() == std::filesystem::file_type::not_found) {
      std::error_code err;
      const auto ok = std::filesystem::create_directories(dir, err);
      if (!ok) {
        std::cerr << "(E) failed to create working directory: " << err.message() << std::endl;
        ::exit(EXIT_FAILURE);
      }
      const auto perms =
        std::filesystem::perms::owner_all |
        std::filesystem::perms::group_all;
      std::filesystem::permissions(dir, perms);
    } else {
      if (status.type() != std::filesystem::file_type::directory) {
        std::cerr << "(E) working directory has been occupied" << std::endl;
        ::exit(EXIT_FAILURE);
      }
      if ((status.permissions() & std::filesystem::perms::owner_all) == std::filesystem::perms::none) {
        std::cerr << "(E) working directory permission denied" << std::endl;
        ::exit(EXIT_FAILURE);
      }
    }
  }

  // gather files
  if (input_paths.empty()) {
    input_paths.push_back(".");
  }
  std::vector<std::string> source_paths;
  for (const auto& path : input_paths) {
    if (std::filesystem::is_regular_file(path)) {
      source_paths.push_back(path);
    } else if (std::filesystem::is_directory(path)) {
      WalkDirectory(
        path,
        [&](const std::filesystem::path& path) {
          return path.filename().string()[0] != '.';
        },
        [&](const std::filesystem::path& path) {
          if (path.has_extension() && std::filesystem::is_regular_file(path)) {
            source_paths.push_back(path.string());
          }
        });
    } else {
      std::cerr << "(E) invalid path: " << path << std::endl;
      ::exit(EXIT_FAILURE);
    }
  }
  std::sort(source_paths.begin(), source_paths.end(), [](const auto& a, const auto& b) {
    return std::strcoll(a.c_str(), b.c_str()) < 0 ? true : false;
  });
  if (source_paths.empty()) {
    std::cout << "(W) no souce files" << std::endl;
    ::exit(EXIT_SUCCESS);
  }

  // identify source files
  std::vector<SourceFile> new_files;
  std::string all_objects;
  {
    std::mutex mutex;
    Semaphore semaphore;
    SourceAnalyzer analyzer(args);
    BlockingQueue<SourceFile> mailbox;
    for (size_t i = 0; i < source_paths.size(); ++i) {
      executor.Push([&, i = i]() {
        auto unit = analyzer.Process(source_paths[i]);
        if (unit) {
          std::lock_guard<std::mutex> locker(mutex);
          if (all_objects.empty()) {
            all_objects = unit.output;
          } else {
            all_objects += " " + unit.output;
          }
          if (!unit.command.empty()) {
            new_files.push_back(std::move(unit));
          }
        }
        semaphore.Post();
      });
    }
    semaphore.Wait(source_paths.size());
  }

  // clean object and and target files
  if (args.at("clean") == "1") {
    const std::string& cmd = "rm -f " + target.string() + ' ' + all_objects;
    std::cout << cmd << std::endl;
    ::system(cmd.c_str());
    ::exit(EXIT_SUCCESS);
  }

  // compile source files
  if (!new_files.empty()) {
    std::cout << "* Build: ";
    if (is_verbose) {
      for (size_t i = 0; i < new_files.size(); ++i) {
        std::cout << new_files[i].source << ((i + 1 < new_files.size()) ? ", " : "");
      }
    } else {
      std::cout << new_files.size() << (new_files.size() > 1 ? " files" : " file");
    }
    std::cout << std::endl;
    {
      int index = 0;
      std::atomic_bool ok = true;
      std::mutex mutex;
      Semaphore semaphore;
      for (size_t i = 0; i < new_files.size(); ++i) {
        executor.Push([&, i = i]() {
          if (ok) {
            const auto& file = new_files[i];
            {
              std::lock_guard<std::mutex> locker(mutex);
              const int percent = static_cast<double>(++index) / new_files.size() * 100;
              std::cout << "[ " << std::setfill(' ') << std::setw(3) << percent << "% ] " << file.Note(is_verbose) << std::endl;
            }
            ok = ::system(file.command.c_str()) == 0;
          }
          semaphore.Post();
        });
      }
      semaphore.Wait(new_files.size());
      if (!ok) {
        ::exit(EXIT_FAILURE);
      }
    }
  }

  // link object files
  if ((!std::filesystem::exists(target) || !new_files.empty()) && args.at("wol") != "1") {
    std::string command =
      JoinStrings({args.at("ld"), args.at("ldflags"), "-o", target.string(), all_objects});
    if (is_verbose) {
      std::cout << "- Link - " << command << std::endl;
    } else {
      std::cout << "- Link - " << target.string() << std::endl;
    }
    if (::system(command.c_str()) != 0) {
      std::cerr << "FATAL: failed to link!" << std::endl;
      std::cerr << "    file:   " << all_objects << std::endl;
      std::cerr << "    ld:     " << args.at("ld") << std::endl;
      std::cerr << "    ldflags: " << args.at("ldflags") << std::endl;
      ::exit(EXIT_FAILURE);
    }
  }

  ::exit(EXIT_SUCCESS);
}
