#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "cab/ArgumentParser.h"
#include "cab/Executor.h"
#include "cab/Semaphore.h"

#include "MakeParser.h"
#include "SourceAnalyzer.h"
#include "Utils.h"

auto main(int argc, char* argv[]) -> int {
  auto result = MakeParser().Parse(argc - 1, argv + 1);
  auto& args = result.args;
  const auto verbose = args.at("verbose") == "1";
  const auto without_link = args.at("wol") == "1";
  const auto target = std::filesystem::path(args.at("workdir")) / std::filesystem::path(args.at("target"));

  // show help
  if (args.at("help") == "1") {
    cab::ArgumentParser::FormatHelpOptions options{4, 4, "\n"};
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
  if (result.rests.empty()) {
    result.rests.emplace_back(".");
  }
  for (const auto& arg : result.rests) {
    if (std::filesystem::is_regular_file(arg)) {
      source_paths.push_back(arg);
    } else if (std::filesystem::is_directory(arg)) {
      WalkDirectory(
        arg,
        [&](const std::filesystem::path& path) {
          return path.filename().string()[0] != '.';
        },
        [&](const std::filesystem::path& path) {
          if (path.has_extension() && std::filesystem::is_regular_file(path)) {
            source_paths.push_back(path.string());
          }
        });
    } else {
      std::cerr << "(E) invalid option or path: " << arg << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }
  SortStrings(source_paths);
  if (source_paths.empty()) {
    std::cout << "(W) no souce files" << std::endl;
    std::exit(EXIT_SUCCESS);
  }

  cab::Executor executor;
  executor.Start(std::stoul(args.at("jobs")));

  // analyze source files
  std::vector<SourceFile> new_files;
  std::vector<std::string> all_outputs;
  auto linker = Linker::ForLd(args.at("ld"));
  {
    std::mutex mutex;
    cab::Semaphore semaphore;
    SourceAnalyzer analyzer(args);
    for (size_t i = 0; i < source_paths.size(); ++i) {
      executor.Push([&, i = i]() {
        auto file = analyzer.Process(source_paths[i]);
        if (file) {
          std::lock_guard<std::mutex> locker(mutex);
          all_outputs.push_back(file.output);
          if (file.linker > linker) {
            linker = file.linker;
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
  SortStrings(all_outputs);
  if (linker) {
    args.at("ld") = linker.command;
  }

  // clean object and and target files
  if (args.at("clean") == "1") {
    const auto& objects = JoinStrings(all_outputs);
    const auto& command = "rm -f " + target.string() + ' ' + objects;
    std::cout << command << std::endl;
    std::system(command.c_str());
    std::exit(EXIT_SUCCESS);
  }

  // compile source files
  if (!new_files.empty()) {
    std::vector<std::string> sources(new_files.size());
    std::transform(new_files.begin(), new_files.end(), sources.begin(), [](const auto& item) {
      return item.source;
    });
    const auto& text = verbose ? JoinStrings(sources) : (std::to_string(new_files.size()) + " file(s)");
    std::cout << "Build " << text << std::endl;
    {
      const size_t total = new_files.size() + (without_link ? 0 : 1);
      size_t current = 0;
      std::atomic_size_t failed = 0;
      std::mutex mutex;
      cab::Semaphore semaphore;
      for (size_t i = 0; i < new_files.size(); ++i) {
        executor.Push([&, i = i]() {
          if (failed == 0) {
            const auto& file = new_files[i];
            const auto& text = verbose ? file.command : (file.source + " => " + file.output);
            {
              std::lock_guard<std::mutex> locker(mutex);
              const auto percentage = ++current * 100 / total;
              std::cout << "[ " << std::setfill(' ') << std::setw(3) << percentage << "% ] " << text << std::endl;
            }
            const auto ok = std::system(file.command.c_str()) == 0;
            if (!ok) {
              ++failed;
            }
          }
          semaphore.Post();
        });
      }
      semaphore.Wait(new_files.size());
      if (failed > 0) {
        std::exit(EXIT_FAILURE);
      }
    }
  }

  // link object files
  if (!without_link && (!new_files.empty() || !std::filesystem::exists(target))) {
    const auto& objects = JoinStrings(all_outputs);
    const auto& linker = args.at("ld");
    if (linker.empty()) {
      std::cerr << "(E) undetermined linker" << std::endl;
      std::exit(EXIT_FAILURE);
    }
    const auto& command =
      JoinStrings({linker, args.at("ldflags"), "-o", target.string(), objects});
    const auto& text = verbose ? command : target.string();
    std::cout << "[ 100% ] " << text << std::endl;
    if (std::system(command.c_str()) != 0) {
      std::cerr << "(E) failed to link" << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  std::exit(EXIT_SUCCESS);
}
