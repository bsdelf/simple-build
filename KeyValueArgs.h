#pragma once

#include <functional>
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
      std::function<void (Args&, std::function<Command (const std::string&)>, std::string)> args_updater;

      // empty command
      Command() = default;

      // normal command
      template <class Updater>
      Command(const std::string& key, const std::string& help, Updater value_updater, const std::string& value)
        : key(key), value(value), help(help), value_updater(value_updater) {
      }

      // value-less command
      template <class Updater>
      Command(const std::string& key, const std::string& help, Updater args_updater)
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
      return [=](Args& args, std::function<Command (const std::string&)> find_cmd, std::string value) {
        for (const auto& key: keys) {
          // forward value-less command
          auto arg_iter = args.find(key);
          if (arg_iter == args.end()) {
            auto cmd = find_cmd(key);
            if (cmd && cmd.args_updater) {
              cmd.args_updater(args, find_cmd, value);
            }
            continue;
          }
          // update value
          auto& to_value = arg_iter->second;
          if (to_value.empty()) {
            to_value = value;
          } else {
            to_value += ' ' + value;
          }
        }
      };
    }

    static auto KeyValueJointer(const std::vector<std::pair<std::string, std::string>>& key_values) {
      return [=](Args& args, std::function<Command (const std::string&)> find_cmd, std::string) {
        for (const auto& key_value: key_values) {
          auto key = key_value.first;
          auto value = key_value.second;
          // forward value-less command
          auto arg_iter = args.find(key);
          if (arg_iter == args.end()) {
            auto cmd = find_cmd(key);
            if (cmd && cmd.args_updater) {
              cmd.args_updater(args, find_cmd, value);
            }
            continue;
          }
          // update value
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
      return [=](Args& args, std::function<Command (const std::string&)> find_cmd, std::string value) {
        for (const auto& key_value: key_values) {
          auto key = key_value.first;
          auto new_value = key_value.second(value);
          // forward value-less command
          auto arg_iter = args.find(key);
          if (arg_iter == args.end()) {
            auto cmd = find_cmd(key);
            if (cmd && cmd.args_updater) {
              cmd.args_updater(args, find_cmd, new_value);
            }
            continue;
          }
          // update value
          auto& to_value = arg_iter->second;
          if (to_value.empty()) {
            to_value = new_value;
          } else {
            to_value += ' ' + new_value;
          }
        }
      };
    }

    template <class OnUnknown>
    static auto Parse(int argc, char** argv, const std::vector<Command>& cmds, OnUnknown on_unknown) {
        Args args;
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

        auto find_cmd = [&](const std::string& key) {
          auto iter = indexed_cmds.find(key);
          if (iter == indexed_cmds.end()) {
            return Command();
          }
          return cmds[iter->second];
        };

        auto UpdateValue = [&](std::string&& key, std::string&& value) {
            const auto& cmd = find_cmd(key);
            if (!cmd) {
                on_unknown(std::move(key), std::move(value));
                return;
            }
            if (cmd.value_updater) {
              auto& current_value = args.at(cmd.key);
              cmd.value_updater(current_value, std::move(value));
            } else if (cmd.args_updater) {
              cmd.args_updater(args, find_cmd, std::move(value));
            }
        };

        for (int i = 1; i < argc; ++i) {
            std::string arg(argv[i]);
            if (arg.empty()) {
              continue;
            }
            auto pos = std::min(arg.find('='), arg.size());
            if (pos < arg.size()) {
                UpdateValue(arg.substr(0, pos), arg.substr(pos + 1));
            } else {
                UpdateValue(std::move(arg), "");
            }
        }
        return args;
    }

    static std::string ToString(const std::vector<Command>& cmds, int space = 0) {
      size_t max = 0;
      for (const auto& cmd: cmds) {
        auto size = cmd.key.size();
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
        const std::string str1(max - cmd.key.size() + 4, ' ');
        result += str0 + cmd.key + str1 + cmd.help + '\n';
      }
      return result;
    }
};
