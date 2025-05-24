#pragma once

#include "db.h"

#include <expected>
#include <netinet/in.h>
#include <variant>

namespace repo {
  struct NoLaunchers {};
  struct Not3Launchers {};
  std::expected<
    std::array<sockaddr_in, 3>,
    std::variant<NoLaunchers, Not3Launchers, std::string_view>
  > get3Launchers(const db::PGconnUR& conn);

  std::expected<void, std::vector<std::string_view>>
  init(const db::PGconnUR& conn);
}
