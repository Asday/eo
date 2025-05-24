#pragma once

#include <string_view>

namespace lg {
  void init();

  void debug(std::string_view name, std::string_view message);
  void info(std::string_view name, std::string_view message);
  void warning(std::string_view name, std::string_view message);
  void fatal(std::string_view name, std::string_view message);
}
