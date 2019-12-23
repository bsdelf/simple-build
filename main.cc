#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "ArgumentParser.h"
#include "Executor.h"
#include "MakeParser.h"
#include "SourceAnalyzer.h"
#include "Utils.h"

#include "Semaphore.h"
using namespace ccbb;

int main(int argc, char* argv[]) {
  std::vector<std::string> input_paths;

  auto args =
    MakeParser()
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
          std::exit(EXIT_FAILURE);
        }
      })
      .Parse(argc, argv)
      .Data();

  const auto is_verbose = args.at("verbose") == "1";
  const auto without_link = args.at("wol") == "1";
  const auto target = std::filesystem::path(args.at("workdir")) / std::filesystem::path(args.at("target"));

  // show help
  if (args.at("help") == "1") {
    ArgumentParser::FormatHelpOptions options{4, 4, "\n"};
    std::cout
      << "Usage:\n\n"
      << std::string(options.space_before_key, ' ') << std::string(argv[0]) << " [options...] [files...] [directories...]\n\n"
      << "Options:\n\n"
      << MakeParser().FormatHelp(options)
      << std::endl;
    std::exit(EXIT_SUCCESS);
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
        std::exit(EXIT_FAILURE);
      }
      const auto perms =
        std::filesystem::perms::owner_all |
        std::filesystem::perms::group_all;
      std::filesystem::permissions(dir, perms);
    } else {
      if (status.type() != std::filesystem::file_type::directory) {
        std::cerr << "(E) working directory has been occupied" << std::endl;
        std::exit(EXIT_FAILURE);
      }
      if ((status.permissions() & std::filesystem::perms::owner_all) == std::filesystem::perms::none) {
        std::cerr << "(E) working directory permission denied" << std::endl;
        std::exit(EXIT_FAILURE);
      }
    }
  }

  // gather source files
  std::vector<std::string> source_paths;
  if (input_paths.empty()) {
    input_paths.push_back(".");
  }
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
      std::exit(EXIT_FAILURE);
    }
  }
  std::sort(source_paths.begin(), source_paths.end(), [](const auto& a, const auto& b) {
    return std::strcoll(a.c_str(), b.c_str()) < 0 ? true : false;
  });
  if (source_paths.empty()) {
    std::cout << "(W) no souce files" << std::endl;
    std::exit(EXIT_SUCCESS);
  }

  Executor executor;
  executor.Start(std::stoul(args.at("jobs")));

  // analyze source files
  std::vector<SourceFile> new_files;
  std::string all_objects;
  {
    std::mutex mutex;
    Semaphore semaphore;
    SourceAnalyzer analyzer(args);
    for (size_t i = 0; i < source_paths.size(); ++i) {
      executor.Push([&, i = i]() {
        auto file = analyzer.Process(source_paths[i]);
        if (file) {
          std::lock_guard<std::mutex> locker(mutex);
          if (all_objects.empty()) {
            all_objects = file.output;
          } else {
            all_objects += " " + file.output;
          }
          if (!file.command.empty()) {
            new_files.push_back(std::move(file));
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
    std::system(cmd.c_str());
    std::exit(EXIT_SUCCESS);
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
      size_t current = 0;
      const size_t total = new_files.size() + (without_link ? 0 : 1);
      std::atomic_bool ok = true;
      std::mutex mutex;
      Semaphore semaphore;
      for (size_t i = 0; i < new_files.size(); ++i) {
        executor.Push([&, i = i]() {
          if (ok) {
            const auto& file = new_files[i];
            {
              std::lock_guard<std::mutex> locker(mutex);
              const int percent = ++current * 100 / total;
              std::cout << "[ " << std::setfill(' ') << std::setw(3) << percent << "% ] ";
              if (is_verbose) {
                std::cout << file.command;
              } else {
                std::cout << (file.source + " => " + file.output);
              }
              std::cout << std::endl;
            }
            ok = std::system(file.command.c_str()) == 0;
          }
          semaphore.Post();
        });
      }
      semaphore.Wait(new_files.size());
      if (!ok) {
        std::exit(EXIT_FAILURE);
      }
    }
  }

  // link object files
  if (!without_link && (!new_files.empty() || !std::filesystem::exists(target))) {
    std::string command =
      JoinStrings({args.at("ld"), args.at("ldflags"), "-o", target.string(), all_objects});
    if (is_verbose) {
      std::cout << "[ 100% ] " << command << std::endl;
    } else {
      std::cout << "[ 100% ] " << target.string() << std::endl;
    }
    if (std::system(command.c_str()) != 0) {
      std::cerr << "FATAL: failed to link!" << std::endl;
      std::cerr << "    file:   " << all_objects << std::endl;
      std::cerr << "    ld:     " << args.at("ld") << std::endl;
      std::cerr << "    ldflags: " << args.at("ldflags") << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  std::exit(EXIT_SUCCESS);
}
