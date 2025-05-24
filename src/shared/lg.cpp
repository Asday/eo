#include "lg.h"

#include <cstdint>
#include <iostream>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

enum class Level : std::uint8_t {
  debug = 0,
  info = 1,
  warning = 2,
  fatal = 3,

  LAST = fatal
};

static constexpr std::array<Level, std::to_underlying(Level::LAST) + 1>
levels{{
  Level::debug,
  Level::info,
  Level::warning,
  Level::fatal
}};

namespace colors {
  namespace fg {
    static constexpr std::string_view yellow{"\033[1;33m"};
    static constexpr std::string_view cyan{"\033[1;36m"};
  }
  namespace bg {
    static constexpr std::string_view red{"\033[1;41m"};
  }
  static constexpr std::string_view reset{"\033[0m"};
}

std::ostream& operator<<(std::ostream& os, const Level l) {
  switch (l) {
    using enum Level;
    case debug: os << "debug"; break;
    case info: os << "info"; break;
    case warning: os << colors::fg::yellow << "warning" << colors::reset; break;
    case fatal: os << colors::bg::red << "fatal" << colors::reset; break;
    default: std::unreachable();
  }

  return os;
}

constexpr std::string_view enabledVar(const Level l) {
  switch (l) {
    using enum Level;
    case debug: return "EO_LOGCONFIG_ENABLED_DEBUG";
    case info: return "EO_LOGCONFIG_ENABLED_INFO";
    case warning: return "EO_LOGCONFIG_ENABLED_WARNING";
    case fatal: return "EO_LOGCONFIG_ENABLED_FATAL";
    default: std::unreachable();
  }
}

constexpr std::string_view enabledNamespacesVar(const Level l) {
  switch (l) {
    using enum Level;
    case debug: return "EO_LOGCONFIG_ENABLED_NAMESPACES_DEBUG";
    case info: return "EO_LOGCONFIG_ENABLED_NAMESPACES_INFO";
    case warning: return "EO_LOGCONFIG_ENABLED_NAMESPACES_WARNING";
    case fatal: return "EO_LOGCONFIG_ENABLED_NAMESPACES_FATAL";
    default: std::unreachable();
  }
}

constexpr std::string_view disabledNamespacesVar(const Level l) {
  switch (l) {
    using enum Level;
    case debug: return "EO_LOGCONFIG_DISABLED_NAMESPACES_DEBUG";
    case info: return "EO_LOGCONFIG_DISABLED_NAMESPACES_INFO";
    case warning: return "EO_LOGCONFIG_DISABLED_NAMESPACES_WARNING";
    case fatal: return "EO_LOGCONFIG_DISABLED_NAMESPACES_FATAL";
    default: std::unreachable();
  }
}

using Namespaces = std::vector<std::string>;
struct LogConfig {
  std::array<bool, std::to_underlying(Level::LAST) + 1> enabledLevels;
  std::array<Namespaces, std::to_underlying(Level::LAST) + 1>
  enabledNamespaces;
  std::array<Namespaces, std::to_underlying(Level::LAST) + 1>
  disabledNamespaces;
};

static LogConfig logConfig{
  .enabledLevels{{false, false, false, true}},
  .enabledNamespaces{{{""}, {""}, {""}, {""}}},
  .disabledNamespaces{{{}, {}, {}, {}}}
};

std::ostream& operator<<(std::ostream& os, const LogConfig& lc) {
  os << "log config\nenabled levels:";
  bool found{false};
  for (const Level l : levels) {
    if (lc.enabledLevels[std::to_underlying(l)]) {
      os << " " << l;
      found = true;
    }
  }
  if (!found) {
    os << " none";

    return os;
  }

  found = false;
  os << "\nenabled namespaces:\n";
  for (const Level l : levels) {
    if (!lc.enabledLevels[std::to_underlying(l)]) continue;
    os << "  " << l << ":";
    for (const auto& ns : lc.enabledNamespaces[std::to_underlying(l)]) {
      if (ns == "") { found = true; os << " all"; break; }

      os << "\n    " << ns;
      found = true;
    }
    if (!found) os << " none";
    os << '\n';
    found = false;
  }

  found = false;
  os << "disabled namespaces (take precedence):";
  for (const Level l : levels) {
    if (!lc.enabledLevels[std::to_underlying(l)]) continue;
    os << "\n  " << l << ":";
    for (const auto& ns : lc.disabledNamespaces[std::to_underlying(l)]) {
      os << "\n    " << ns;
      found = true;
    }
    if (!found) os << " none";
  }

  return os;
}

inline bool isEnabled(Level l, std::string_view name) {
  if (!logConfig.enabledLevels[std::to_underlying(l)]) return false;

  for (const auto& ns : logConfig.disabledNamespaces[std::to_underlying(l)]) {
    if (name.starts_with(ns)) return false;
  }

  for (const auto& ns : logConfig.enabledNamespaces[std::to_underlying(l)]) {
    if (name.starts_with(ns)) return true;
  }

  return false;
}

void log(Level level, std::string_view name, std::string_view message) {
  if (!isEnabled(level, name)) return;
  std::clog
    << name
    << colors::fg::cyan
    << " | "
    << colors::reset
    << level
    << ": "
    << message
    << std::endl
  ;
}

void lg::debug(std::string_view name, std::string_view message) {
  log(Level::debug, name, message);
}

void lg::info(std::string_view name, std::string_view message) {
  log(Level::info, name, message);
}

void lg::warning(std::string_view name, std::string_view message) {
  log(Level::warning, name, message);
}

void lg::fatal(std::string_view name, std::string_view message) {
  log(Level::fatal, name, message);
}

void sortAndSimplify([[maybe_unused]] Namespaces& n) {
  // TODO: sort lexicographically, then remove all items whose prefix
  // appears earlier in the `vector`.
}

void lg::init() {
  std::string_view conf{};
  std::stringstream confStream{};
  std::string word{};
  // TODO: add support for env vars that drop the `Level` suffix and
  // instead apply to all `Level`s.
  for (const Level l : levels) {
    do {
      char* rawConf{getenv(enabledVar(l).data())};
      if (rawConf == NULL) break;
      conf = rawConf;
      if (conf == "true") {
        logConfig.enabledLevels[std::to_underlying(l)] = true;
      } else if (conf == "false") {
        logConfig.enabledLevels[std::to_underlying(l)] = false;
      } else {
        std::clog
      	  << "`"
      	  << enabledVar(l)
      	  << "` must be `true` or `false`, leaving as default"
      	  << std::endl;
        ;
      }
    } while (false);

    Namespaces& enabled{
      logConfig.enabledNamespaces[std::to_underlying(l)]
    };
    do {
      char* rawConf{getenv(enabledNamespacesVar(l).data())};
      if (rawConf == NULL) break;
      confStream.str(rawConf);
      while (std::getline(confStream, word, ';')) {
        enabled.push_back(word);
      }
    } while (false);
    if (enabled.size() > 1) enabled.erase(enabled.begin());
    sortAndSimplify(enabled);

    Namespaces disabled{
      logConfig.disabledNamespaces[std::to_underlying(l)]
    };
    do {
      char* rawConf{getenv(disabledNamespacesVar(l).data())};
      if (rawConf == NULL) break;
      confStream.str(rawConf);
      confStream.clear();
      confStream.seekg(0);
      while (std::getline(confStream, word, ';')) {
        logConfig.disabledNamespaces[std::to_underlying(l)].push_back(word);
      }
    } while (false);
    sortAndSimplify(disabled);
  }

  lg::debug("lg", (std::stringstream()
    << "initialised logging with config:\n"
    << logConfig
  ).view());
}
