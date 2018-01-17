#pragma once

#include <string>
#include <vector>
#include <unordered_map>

class KeyValueArgs {
  public:
    using Args = std::unordered_map<std::string, std::string>;

    struct Command {
      std::string key;
      std::string value;
      std::string help;
      std::function<void (std::string&, std::string)> value_updater;
      std::function<void (std::unordered_map<std::string, std::string>&, std::string)> args_updater;

      Command() = default;

      Command(const std::string& key, const std::string& help, std::function<void (std::string&, std::string)> value_updater, const std::string& value)
        : key(key), value(value), help(help), value_updater(value_updater) {
      }

      Command(const std::string& key, const std::string& help, std::function<void (std::unordered_map<std::string, std::string>&, std::string)> args_updater)
        : key(key), help(help), args_updater(args_updater) {
      }

      explicit operator bool() const {
        return !key.empty();
      }
    };

    static auto Setter() {
      return [=](std::string& to_value, std::string value) {
        to_value = value;
      };
    }

    static auto ValueSetter(const std::string& value) {
      return [=](std::string& to_value, std::string) {
        to_value = value;
      };
    }

    static auto Jointer() {
      return [=](std::string& to_value, std::string value) {
        if (to_value.empty()) {
          to_value = value;
        } else {
          to_value += ' ' + value;
        }
      };
    }

    static auto KeyJointer(const std::vector<std::string>& keys) {
      return [=](std::unordered_map<std::string, std::string>& args, std::string value) {
        for (const auto& key: keys) {
          auto& to_value = args.at(key);
          if (to_value.empty()) {
            to_value = value;
          } else {
            to_value += ' ' + value;
          }
        }
      };
    }

    static auto KeyValueJointer(const std::vector<std::pair<std::string, std::string>>& key_values) {
      return [=](std::unordered_map<std::string, std::string>& args, std::string) {
        for (const auto& key_value: key_values) {
          auto key = key_value.first;
          auto value = key_value.second;
          auto arg_iter = args.find(key);
          if (arg_iter == args.end()) {
            // need forward
            return;
          }
          auto& to_value = arg_iter->second;
          if (to_value.empty()) {
            to_value = value;
          } else {
            to_value += ' ' + value;
          }
        }
      };
    }

    static auto KeyValueJointer(const std::vector<std::pair<std::string, std::function<std::string (const std::string&)>>>& key_values) {
      return [=](std::unordered_map<std::string, std::string>& args, std::string value) {
        for (const auto& key_value: key_values) {
          auto key = key_value.first;
          auto new_value = key_value.second(value);
          auto& to_value = args.at(key);
          if (to_value.empty()) {
            to_value = new_value;
          } else {
            to_value += ' ' + new_value;
          }
        }
      };
    }

    template <class OnUnknown>
    static auto Parse(int argc, char** argv, const std::vector<Command>& cmds, OnUnknown on_unknown) -> std::unordered_map<std::string, std::string> {
      std::unordered_map<std::string, std::string> args;
      std::unordered_map<std::string, size_t> indexed_cmds;
        for (size_t i = 0; i < cmds.size(); ++i) {
          const auto& cmd = cmds[i];
          if (cmd) {
            if (cmd.value_updater) {
              args.emplace(cmd.key, cmd.value);
            }
            indexed_cmds.emplace(cmd.key, i);
          }
        }

        auto UpdateValue = [&](std::string key, std::string value = "") {
            const auto indexed_cmd_iter = indexed_cmds.find(key);
            if (indexed_cmd_iter == indexed_cmds.end()) {
                on_unknown(key, value);
                return;
            }
            const auto& cmd = cmds[indexed_cmd_iter->second];
            if (cmd.value_updater) {
              auto& current_value = args.at(cmd.key);
              cmd.value_updater(current_value, std::move(value));
            } else if (cmd.args_updater) {
              cmd.args_updater(args, std::move(value));
            }
        };

        bool cont = true;
        for (int i = 1; i < argc && cont; ++i) {
            std::string arg(argv[i]);
            auto pos = arg.find('=');
            if (pos != std::string::npos && pos != arg.size() - 1) {
                UpdateValue(arg.substr(0, pos), arg.substr(pos + 1));
            } else {
                UpdateValue(std::move(arg));
            }
        }
        return args;
    }

    static std::string ToString(const std::vector<Command>& cmds, int space = 0) {
      size_t max = 0;
      for (const auto& cmd: cmds) {
        auto size = cmd.key.size() + (cmd.value_updater ? 2 : 0);
        if (size > max) {
          max = size;
        }
      }
      const std::string str0(space, ' ');
      std::string result;
      for (const auto& cmd: cmds) {
        if (cmd.key.empty()) {
          result += '\n';
          continue;
        }
        const std::string str1 = cmd.value_updater ? "=?": "";
        const std::string str2(max - cmd.key.size()  - str1.size() + 4, ' ');
        result += str0 + cmd.key + str1 + str2 + cmd.help + '\n';
      }
      return result;
    }
};
