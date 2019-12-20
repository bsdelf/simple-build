#pragma once

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

class ArgumentParser {
 public:
  ArgumentParser& Parse(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
      std::string arg(argv[i]);
      if (arg.empty()) {
        continue;
      }
      std::string key;
      std::optional<std::string> value;
      const auto pos = std::min(arg.find('='), arg.size());
      if (pos < arg.size()) {
        key = arg.substr(0, pos);
        value = arg.substr(pos + 1);
      } else {
        key = std::move(arg);
      }
      const auto& iter = on_keys_.find(key);
      if (iter == on_keys_.end()) {
        if (on_unknown_) {
          on_unknown_(std::move(key), std::move(value));
        }
      } else {
        const auto& on_key = iter->second;
        for (size_t i = 0; i < on_key.size(); ++i) {
          on_key[i](value);
        }
      }
    }
    return *this;
  }

  struct Set {
    std::optional<std::string> initial_value;
    std::optional<std::string> default_value;
    std::function<void(std::string&)> modifier;
    Set(std::optional<std::string> initial_value, std::optional<std::string> default_value)
      : initial_value(std::move(initial_value)), default_value(std::move(default_value)) {
    }
    Set(std::optional<std::string> initial_value, std::optional<std::string> default_value, std::function<void(std::string&)> modifier)
      : initial_value(std::move(initial_value)), default_value(std::move(default_value)), modifier(std::move(modifier)) {
    }
  };

  template <class... T>
  ArgumentParser& On(const std::string& key, const std::string& help, Set set, T... rest) {
    key_helps_.emplace_back(key, help);
    return On(key, set, rest...);
  }

  ArgumentParser& On(const std::string& key, Set set) {
    if (set.initial_value) {
      args_[key] = *set.initial_value;
    }
    auto handler =
      ([this, key = key,
        default_value = set.default_value,
        modifier = set.modifier](std::optional<std::string> input_value) {
        auto actual_value = input_value ? input_value : default_value;
        if (actual_value) {
          auto value = std::move(*actual_value);
          if (modifier) {
            modifier(value);
          }
          auto iter = args_.find(key);
          if (iter == args_.end()) {
            args_.emplace(key, std::move(value));
          } else {
            iter->second = std::move(value);
          }
        }
      });
    on_keys_[key].push_back(std::move(handler));
    return *this;
  }

  template <class... T>
  ArgumentParser& On(const std::string& key, Set set, T... rest) {
    return On(key, set).On(key, rest...);
  }

  struct SetTo {
    std::string key;
    std::optional<std::string> initial_value;
    std::optional<std::string> default_value;
    std::function<void(std::string&)> modifier;
    SetTo(const std::string& key,
          std::optional<std::string> initial_value,
          std::optional<std::string> default_value)
      : key(key), initial_value(std::move(initial_value)), default_value(std::move(default_value)) {
    }
    SetTo(const std::string& key,
          std::optional<std::string> initial_value,
          std::optional<std::string> default_value,
          std::function<void(std::string&)> modifier)
      : key(key), initial_value(std::move(initial_value)), default_value(std::move(default_value)), modifier(std::move(modifier)) {
    }
  };

  template <class... T>
  ArgumentParser& On(const std::string& key, const std::string& help, SetTo set_to, T... rest) {
    key_helps_.emplace_back(key, help);
    return On(key, set_to, rest...);
  }

  ArgumentParser& On(const std::string& key, SetTo set_to) {
    auto to_key = set_to.key;
    if (set_to.initial_value) {
      args_[to_key] = std::move(*set_to.initial_value);
    }
    auto handler =
      ([this,
        to_key = to_key,
        default_value = set_to.default_value,
        modifier = set_to.modifier](std::optional<std::string> input_value) {
        auto actual_value = input_value ? input_value : default_value;
        if (actual_value) {
          auto value = std::move(*actual_value);
          if (modifier) {
            modifier(value);
          }
          auto iter = args_.find(to_key);
          if (iter == args_.end()) {
            args_.emplace(to_key, std::move(value));
          } else {
            iter->second = std::move(value);
          }
        }
      });
    on_keys_[key].push_back(std::move(handler));
    return *this;
  }

  template <class... T>
  ArgumentParser& On(const std::string& key, SetTo set_to, T... rest) {
    return On(key, set_to).On(key, rest...);
  }

  struct Join {
    std::optional<std::string> initial_value;
    std::optional<std::string> default_value;
    std::string separator = " ";
    std::function<void(std::string&)> modifier;
    Join(std::optional<std::string> initial_value,
         std::optional<std::string> default_value)
      : initial_value(std::move(initial_value)),
        default_value(std::move(default_value)) {
    }
    Join(std::optional<std::string> initial_value,
         std::optional<std::string> default_value,
         const std::string& separator)
      : initial_value(std::move(initial_value)),
        default_value(std::move(default_value)),
        separator(separator) {
    }
    Join(std::optional<std::string> initial_value,
         std::optional<std::string> default_value,
         std::function<void(std::string&)> modifier)
      : initial_value(std::move(initial_value)),
        default_value(std::move(default_value)),
        modifier(std::move(modifier)) {
    }
    Join(std::optional<std::string> initial_value,
         std::optional<std::string> default_value,
         const std::string& separator,
         std::function<void(std::string&)> modifier)
      : initial_value(std::move(initial_value)),
        default_value(std::move(default_value)),
        separator(separator),
        modifier(std::move(modifier)) {
    }
  };

  template <class... T>
  ArgumentParser& On(const std::string& key, const std::string& help, Join join, T... rest) {
    key_helps_.emplace_back(key, help);
    return On(key, join, rest...);
  }

  ArgumentParser& On(const std::string& key, Join join) {
    auto separator = join.separator;
    if (join.initial_value) {
      auto iter = args_.find(key);
      if (iter == args_.end()) {
        args_.emplace(key, std::move(*join.initial_value));
      } else {
        iter->second = iter->second + separator + std::move(*join.initial_value);
      }
    }
    auto handler =
      ([this,
        key = key,
        separator = separator,
        default_value = join.default_value,
        modifier = join.modifier](std::optional<std::string> input_value) {
        auto actual_value = input_value ? input_value : default_value;
        if (actual_value) {
          auto value = std::move(*actual_value);
          if (modifier) {
            modifier(value);
          }
          auto iter = args_.find(key);
          if (iter == args_.end()) {
            args_.emplace(key, std::move(value));
          } else {
            iter->second = iter->second + separator + value;
          }
        }
      });
    on_keys_[key].push_back(std::move(handler));
    return *this;
  }

  template <class... T>
  ArgumentParser& On(const std::string& key, Join join, T... rest) {
    return On(key, join).On(key, rest...);
  }

  struct JoinTo {
    std::string key;
    std::optional<std::string> initial_value;
    std::optional<std::string> default_value;
    std::string separator = " ";
    std::function<void(std::string&)> modifier;
    JoinTo(const std::string& key,
           std::optional<std::string> initial_value,
           std::optional<std::string> default_value)
      : key(key),
        initial_value(std::move(initial_value)),
        default_value(std::move(default_value)) {
    }
    JoinTo(const std::string& key,
           std::optional<std::string> initial_value,
           std::optional<std::string> default_value,
           std::function<void(std::string&)> modifier)
      : key(key),
        initial_value(std::move(initial_value)),
        default_value(std::move(default_value)),
        modifier(std::move(modifier)) {
    }
    JoinTo(const std::string& key,
           std::optional<std::string> initial_value,
           std::optional<std::string> default_value,
           const std::string& separator)
      : key(key),
        initial_value(std::move(initial_value)),
        default_value(std::move(default_value)),
        separator(separator) {
    }
    JoinTo(const std::string& key,
           std::optional<std::string> initial_value,
           std::optional<std::string> default_value,
           const std::string& separator,
           std::function<void(std::string&)> modifier)
      : key(key),
        initial_value(std::move(initial_value)),
        default_value(std::move(default_value)),
        separator(separator),
        modifier(std::move(modifier)) {
    }
  };

  template <class... T>
  ArgumentParser& On(const std::string& key, const std::string& help, JoinTo join_to, T... rest) {
    key_helps_.emplace_back(key, help);
    return On(key, join_to, rest...);
  }

  ArgumentParser& On(const std::string& key, JoinTo join_to) {
    auto to_key = join_to.key;
    auto separator = join_to.separator;
    if (join_to.initial_value) {
      auto iter = args_.find(to_key);
      if (iter == args_.end()) {
        args_.emplace(to_key, std::move(*join_to.initial_value));
      } else {
        iter->second = iter->second + separator + *join_to.initial_value;
      }
    }
    auto handler =
      ([this,
        to_key = to_key,
        separator = separator,
        default_value = join_to.default_value,
        modifier = join_to.modifier](std::optional<std::string> input_value) {
        auto actual_value = input_value ? input_value : default_value;
        if (actual_value) {
          auto value = std::move(*actual_value);
          if (modifier) {
            modifier(value);
          }
          auto iter = args_.find(to_key);
          if (iter == args_.end()) {
            args_.emplace(to_key, std::move(value));
          } else {
            iter->second = iter->second + separator + value;
          }
        }
      });
    on_keys_[key].push_back(std::move(handler));
    return *this;
  }

  template <class... T>
  ArgumentParser& On(const std::string& key, JoinTo join_to, T... rest) {
    return On(key, join_to).On(key, rest...);
  }

  ArgumentParser& Split() {
    key_helps_.emplace_back();
    return *this;
  }

  ArgumentParser& OnUnknown(std::function<void(std::string, std::optional<std::string>)> handler) {
    on_unknown_ = std::move(handler);
    return *this;
  }

  void Clear() {
    key_helps_.clear();
    on_keys_.clear();
    on_unknown_ = nullptr;
    args_.clear();
  }

  struct FormatHelpOptions {
    int space_before_key = 0;
    int min_space_before_value = 4;
    std::string newline = "\n";
  };

  std::string FormatHelp(const FormatHelpOptions& options) const {
    std::string result;
    size_t max = 0;
    for (const auto& item : key_helps_) {
      auto size = item.key.size();
      if (size > max) {
        max = size;
      }
    }
    const std::string space_before_key(options.space_before_key, ' ');
    for (const auto& item : key_helps_) {
      if (item.key.empty()) {
        result += options.newline;
        continue;
      }
      std::string space_before_value(max - item.key.size() + options.min_space_before_value, ' ');
      result += space_before_key + item.key + space_before_value + item.help + options.newline;
    }
    return result;
  }

  auto operator[](const std::string& key) const {
    std::optional<std::string> value;
    if (const auto iter = args_.find(key); iter != args_.end()) {
      value = iter->second;
    }
    return value;
  }

  const std::map<std::string, std::string>& Data() const {
    return args_;
  }

 private:
  struct KeyHelp {
    std::string key;
    std::string help;
    KeyHelp() = default;
    KeyHelp(const std::string& key, const std::string& help)
      : key(key), help(help) {
    }
  };
  std::vector<KeyHelp> key_helps_;
  std::map<std::string, std::vector<std::function<void(std::optional<std::string>)>>> on_keys_;
  std::function<void(std::string, std::optional<std::string>)> on_unknown_;
  std::map<std::string, std::string> args_;
};
